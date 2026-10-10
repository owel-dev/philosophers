#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define TOLERANCE_MS 10
#define OBSERVE_MS 3000
#define DEADLINE_MS 15000
#define MAX_ARGS 16

#define CHECK(expr) check((expr), #expr, __LINE__, "")
#define CHECK_MSG(expr, ...) check((expr), #expr, __LINE__, __VA_ARGS__)

typedef struct s_buf
{
	char	*data;
	size_t	len;
	size_t	cap;
}	t_buf;

typedef struct s_event
{
	long		ts;
	int			who;
	const char	*state;
}	t_event;

typedef struct s_run
{
	char	label[64];
	char	*err;
	t_event	*events;
	size_t	event_count;
	size_t	event_cap;
	size_t	line_count;
	char	bad_line[128];
	int		has_bad_line;
	int		timed_out;
	int		exit_code;
	int		sig;
	size_t	all_ate_line;
	int		all_ate_times;
}	t_run;

typedef struct s_timing
{
	long	die;
	long	eat;
	long	sleep;
}	t_timing;

static const char	*g_states[] = {
	"has taken a fork", "is eating", "is sleeping", "is thinking", "died"};

static const char	*g_binary = "./philo";
static int			g_checked;
static int			g_failed;
static int			g_tests;
static int			g_current_failed;
static const char	*g_red = "";
static const char	*g_green = "";
static const char	*g_cyan = "";
static const char	*g_bold = "";
static const char	*g_reset = "";

static void	set_colors(void)
{
	if (!isatty(STDOUT_FILENO) || getenv("NO_COLOR") != NULL)
		return ;
	g_red = "\033[1;31m";
	g_green = "\033[1;32m";
	g_cyan = "\033[1;36m";
	g_bold = "\033[1m";
	g_reset = "\033[0m";
}

static void	die(const char *what)
{
	perror(what);
	exit(2);
}

__attribute__((format(printf, 4, 5)))
static void	check(int ok, const char *expr, int line, const char *fmt, ...)
{
	va_list	ap;

	g_checked++;
	if (ok)
		return ;
	if (!g_current_failed)
		g_failed++;
	g_current_failed = 1;
	printf("  %sFAIL%s %s line %d: %s", g_red, g_reset, __FILE__, line, expr);
	if (fmt[0])
	{
		printf("\n       ");
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
	printf("\n");
}

static long	now_ms(void)
{
	struct timespec	now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000L + now.tv_nsec / 1000000);
}

static void	buf_init(t_buf *buf)
{
	buf->cap = 4096;
	buf->len = 0;
	buf->data = malloc(buf->cap);
	if (buf->data == NULL)
		die("malloc");
	buf->data[0] = '\0';
}

static void	buf_append(t_buf *buf, const char *src, size_t n)
{
	char	*grown;

	if (buf->len + n + 1 > buf->cap)
	{
		buf->cap = (buf->len + n + 1) * 2;
		grown = realloc(buf->data, buf->cap);
		if (grown == NULL)
			die("realloc");
		buf->data = grown;
	}
	memcpy(buf->data + buf->len, src, n);
	buf->len += n;
	buf->data[buf->len] = '\0';
}

static void	push_event(t_run *run, t_event event)
{
	t_event	*grown;

	if (run->event_count == run->event_cap)
	{
		run->event_cap = run->event_cap ? run->event_cap * 2 : 256;
		grown = realloc(run->events, run->event_cap * sizeof *grown);
		if (grown == NULL)
			die("realloc");
		run->events = grown;
	}
	run->events[run->event_count++] = event;
}

static size_t	count_of(const t_run *run, const char *state, int who)
{
	size_t	i;
	size_t	n;

	n = 0;
	i = 0;
	while (i < run->event_count)
	{
		if (strcmp(run->events[i].state, state) == 0
			&& (who == 0 || run->events[i].who == who))
			n++;
		i++;
	}
	return (n);
}

static const t_event	*first_of(const t_run *run, const char *state)
{
	size_t	i;

	i = 0;
	while (i < run->event_count)
	{
		if (strcmp(run->events[i].state, state) == 0)
			return (&run->events[i]);
		i++;
	}
	return (NULL);
}

static long	last_ts_of(const t_run *run, const char *state, int who)
{
	size_t	i;
	long	ts;

	ts = 0;
	i = 0;
	while (i < run->event_count)
	{
		if (strcmp(run->events[i].state, state) == 0 && run->events[i].who == who)
			ts = run->events[i].ts;
		i++;
	}
	return (ts);
}

static int	parse_event(const char *line, t_event *out)
{
	char	*end;
	long	who;
	size_t	i;

	out->ts = strtol(line, &end, 10);
	if (end == line || out->ts < 0)
		return (0);
	line = end;
	who = strtol(line, &end, 10);
	if (end == line || who < 1 || who > INT_MAX || *end != ' ')
		return (0);
	out->who = (int)who;
	line = end + 1;
	i = 0;
	while (i < sizeof g_states / sizeof g_states[0])
	{
		if (strcmp(line, g_states[i]) == 0)
		{
			out->state = g_states[i];
			return (1);
		}
		i++;
	}
	return (0);
}

static int	parse_all_ate(const char *line, int *times)
{
	char	*end;
	long	n;

	n = strtol(line, &end, 10);
	if (end == line || n < 0 || *end != ' ')
		return (0);
	line = end + 1;
	if (strncmp(line, "all philosophers have eaten ", 28) != 0)
		return (0);
	line += 28;
	n = strtol(line, &end, 10);
	if (end == line || n < 0 || n > INT_MAX || strcmp(end, " times") != 0)
		return (0);
	*times = (int)n;
	return (1);
}

static void	take_line(t_run *run, const char *line)
{
	t_event	event;

	run->line_count++;
	if (parse_event(line, &event))
		push_event(run, event);
	else if (parse_all_ate(line, &run->all_ate_times))
		run->all_ate_line = run->line_count;
	else if (!run->has_bad_line)
	{
		run->has_bad_line = 1;
		snprintf(run->bad_line, sizeof run->bad_line, "%s", line);
	}
}

static void	parse_output(t_run *run, char *out)
{
	char	*nl;

	while (*out)
	{
		nl = strchr(out, '\n');
		if (nl == NULL)
		{
			if (!run->timed_out)
				take_line(run, out);
			return ;
		}
		*nl = '\0';
		take_line(run, out);
		out = nl + 1;
	}
}

static int	open_terminal(int *slave)
{
	int				master;
	const char		*name;
	struct termios	raw;

	master = posix_openpt(O_RDWR | O_NOCTTY);
	if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0)
		die("posix_openpt");
	name = ptsname(master);
	*slave = -1;
	if (name != NULL)
		*slave = open(name, O_RDWR | O_NOCTTY);
	if (*slave < 0)
		die("ptsname");
	if (tcgetattr(*slave, &raw) == 0)
	{
		raw.c_oflag &= ~(tcflag_t)OPOST;
		tcsetattr(*slave, TCSANOW, &raw);
	}
	return (master);
}

static void	build_argv(const char *args, char *copy, size_t size, char **argv)
{
	int		argc;
	char	*word;

	snprintf(copy, size, "%s", args);
	argv[0] = (char *)g_binary;
	argc = 1;
	word = strtok(copy, " ");
	while (word != NULL && argc < MAX_ARGS - 1)
	{
		argv[argc++] = word;
		word = strtok(NULL, " ");
	}
	argv[argc] = NULL;
}

static pid_t	spawn(const char *args, int *out_fd, int *err_fd)
{
	char	copy[64];
	char	*argv[MAX_ARGS];
	int		slave;
	int		err[2];
	pid_t	pid;

	build_argv(args, copy, sizeof copy, argv);
	*out_fd = open_terminal(&slave);
	if (pipe(err) != 0)
		die("pipe");
	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
	{
		dup2(slave, STDOUT_FILENO);
		dup2(err[1], STDERR_FILENO);
		close(*out_fd);
		close(slave);
		close(err[0]);
		close(err[1]);
		execv(g_binary, argv);
		perror(g_binary);
		_exit(127);
	}
	close(slave);
	close(err[1]);
	*err_fd = err[0];
	return (pid);
}

static void	read_ready(int fds[2], int open_fd[2], fd_set *set, t_buf sinks[2])
{
	char	buf[4096];
	ssize_t	n;
	int		i;

	i = 0;
	while (i < 2)
	{
		if (open_fd[i] && FD_ISSET(fds[i], set))
		{
			n = read(fds[i], buf, sizeof buf);
			if (n > 0)
				buf_append(&sinks[i], buf, (size_t)n);
			else
				open_fd[i] = 0;
		}
		i++;
	}
}

static void	drain(int fd, t_buf *into)
{
	char	buf[4096];
	ssize_t	n;

	while ((n = read(fd, buf, sizeof buf)) > 0)
		buf_append(into, buf, (size_t)n);
}

static void	collect(t_run *run, pid_t pid, int fds[2], t_buf sinks[2], int limit_ms)
{
	int				open_fd[2];
	long			start;
	long			left;
	fd_set			set;
	struct timeval	wait;
	int				ready;
	int				status;
	int				i;

	open_fd[0] = 1;
	open_fd[1] = 1;
	start = now_ms();
	while (open_fd[0] || open_fd[1])
	{
		left = limit_ms - (now_ms() - start);
		if (left <= 0)
		{
			run->timed_out = 1;
			kill(pid, SIGKILL);
			break ;
		}
		FD_ZERO(&set);
		i = 0;
		while (i < 2)
		{
			if (open_fd[i])
				FD_SET(fds[i], &set);
			i++;
		}
		wait.tv_sec = left / 1000;
		wait.tv_usec = (left % 1000) * 1000;
		ready = select((fds[0] > fds[1] ? fds[0] : fds[1]) + 1, &set, NULL, NULL, &wait);
		if (ready < 0 && errno == EINTR)
			continue ;
		if (ready < 0)
			die("select");
		read_ready(fds, open_fd, &set, sinks);
	}
	status = 0;
	waitpid(pid, &status, 0);
	i = 0;
	while (i < 2)
	{
		if (open_fd[i])
			drain(fds[i], &sinks[i]);
		i++;
	}
	if (WIFEXITED(status))
		run->exit_code = WEXITSTATUS(status);
	else if (WIFSIGNALED(status) && !run->timed_out)
		run->sig = WTERMSIG(status);
}

static t_run	philo(const char *args, int limit_ms)
{
	t_run	run;
	int		fds[2];
	t_buf	sinks[2];
	pid_t	pid;

	memset(&run, 0, sizeof run);
	run.exit_code = -1;
	snprintf(run.label, sizeof run.label, "philo %s", args);
	buf_init(&sinks[0]);
	buf_init(&sinks[1]);
	pid = spawn(args, &fds[0], &fds[1]);
	collect(&run, pid, fds, sinks, limit_ms);
	close(fds[0]);
	close(fds[1]);
	parse_output(&run, sinks[0].data);
	free(sinks[0].data);
	run.err = sinks[1].data;
	return (run);
}

static void	free_run(t_run *run)
{
	free(run->err);
	free(run->events);
}

static void	check_output_is_well_formed(const t_run *run, int count)
{
	size_t	i;
	int		ordered;
	int		in_range;
	size_t	deaths;

	CHECK_MSG(run->err[0] == '\0', "%s: wrote to stderr: %s", run->label, run->err);
	CHECK_MSG(run->sig == 0, "%s: killed by signal %d", run->label, run->sig);
	CHECK_MSG(run->line_count > 0, "%s: printed nothing", run->label);
	CHECK_MSG(!run->has_bad_line, "%s: malformed line: \"%s\"", run->label, run->bad_line);
	ordered = 1;
	in_range = 1;
	i = 0;
	while (i < run->event_count)
	{
		if (i > 0 && run->events[i].ts < run->events[i - 1].ts)
			ordered = 0;
		if (run->events[i].who > count)
			in_range = 0;
		i++;
	}
	CHECK_MSG(ordered, "%s: timestamps go backwards", run->label);
	CHECK_MSG(in_range, "%s: philosopher id above %d", run->label, count);
	deaths = count_of(run, "died", 0);
	CHECK_MSG(deaths <= 1, "%s: %zu death lines", run->label, deaths);
	if (deaths > 0)
		CHECK_MSG(strcmp(run->events[run->event_count - 1].state, "died") == 0,
			"%s: output continues after a death", run->label);
	if (run->all_ate_line > 0)
		CHECK_MSG(run->all_ate_line == run->line_count,
			"%s: output continues after reporting everyone has eaten", run->label);
}

static void	check_death_is_announced_on_time(const t_run *run, long time_to_die)
{
	size_t			deaths;
	const t_event	*died;
	long			delay;
	char			detail[160];

	deaths = count_of(run, "died", 0);
	CHECK_MSG(deaths == 1, "%s: expected exactly one death, got %zu", run->label, deaths);
	if (deaths != 1)
		return ;
	died = first_of(run, "died");
	delay = died->ts - last_ts_of(run, "is eating", died->who);
	snprintf(detail, sizeof detail,
		"%s: philosopher %d died %ldms after its last meal, time_to_die is %ld",
		run->label, died->who, delay, time_to_die);
	CHECK_MSG(delay <= time_to_die + TOLERANCE_MS, "%s", detail);
	CHECK_MSG(delay >= time_to_die - TOLERANCE_MS, "%s", detail);
}

static void	scenario(const char *args, const char *expectation)
{
	g_tests++;
	g_current_failed = 0;
	printf("  %s%-18s%s %s\n", g_cyan, args, g_reset, expectation);
	fflush(stdout);
}

static void	철학자_1명_먹지_못하고_죽기(void)
{
	const char	*args = "1 800 200 200";
	t_run		run;

	scenario(args, "the philosopher does not eat and dies");
	run = philo(args, 5000);
	check_output_is_well_formed(&run, 1);
	CHECK_MSG(!run.timed_out, "%s: still running, no death was detected", run.label);
	CHECK_MSG(run.exit_code == 0, "%s: exit code %d", run.label, run.exit_code);
	CHECK_MSG(count_of(&run, "is eating", 0) == 0, "%s: ate with a single fork", run.label);
	check_death_is_announced_on_time(&run, 800);
	free_run(&run);
}

static void	철학자_5명_아무도_죽지_않기(void)
{
	const char	*args = "5 800 200 200";
	t_run		run;

	scenario(args, "nobody dies");
	run = philo(args, OBSERVE_MS);
	check_output_is_well_formed(&run, 5);
	CHECK_MSG(run.timed_out, "%s: stopped on its own, exit code %d", run.label, run.exit_code);
	CHECK_MSG(count_of(&run, "died", 0) == 0, "%s: somebody died", run.label);
	free_run(&run);
}

static void	철학자_5명_모두_7번_먹으면_멈추기(void)
{
	const char	*args = "5 800 200 200 7";
	t_run		run;
	size_t		meals;
	int			who;

	scenario(args, "nobody dies, reports everyone has eaten 7 times and stops");
	run = philo(args, DEADLINE_MS);
	check_output_is_well_formed(&run, 5);
	CHECK_MSG(!run.timed_out, "%s: did not stop on its own", run.label);
	CHECK_MSG(run.exit_code == 0, "%s: exit code %d", run.label, run.exit_code);
	CHECK_MSG(count_of(&run, "died", 0) == 0, "%s: somebody died", run.label);
	CHECK_MSG(run.all_ate_line > 0, "%s: did not report that everyone has eaten", run.label);
	if (run.all_ate_line > 0)
		CHECK_MSG(run.all_ate_times == 7, "%s: reported %d meals instead of 7",
			run.label, run.all_ate_times);
	who = 1;
	while (who <= 5)
	{
		meals = count_of(&run, "is eating", who);
		CHECK_MSG(meals >= 7, "%s: philosopher %d ate %zu times when the table stopped",
			run.label, who, meals);
		who++;
	}
	free_run(&run);
}

static void	철학자_4명_빠듯한_일정에서_아무도_죽지_않기(void)
{
	const char	*args = "4 410 200 200";
	t_run		run;

	scenario(args, "nobody dies");
	run = philo(args, OBSERVE_MS);
	check_output_is_well_formed(&run, 4);
	CHECK_MSG(run.timed_out, "%s: stopped on its own, exit code %d", run.label, run.exit_code);
	CHECK_MSG(count_of(&run, "died", 0) == 0, "%s: somebody died", run.label);
	free_run(&run);
}

static void	철학자_4명_중_한_명_죽기(void)
{
	const char	*args = "4 310 200 100";
	t_run		run;

	scenario(args, "one philosopher dies");
	run = philo(args, 5000);
	check_output_is_well_formed(&run, 4);
	CHECK_MSG(!run.timed_out, "%s: still running, no death was detected", run.label);
	CHECK_MSG(run.exit_code == 0, "%s: exit code %d", run.label, run.exit_code);
	check_death_is_announced_on_time(&run, 310);
	free_run(&run);
}

static void	철학자_2명_사망_10ms_안에_출력하기(void)
{
	static const t_timing	timings[] = {{310, 200, 100}, {410, 300, 100}, {610, 400, 200}};
	char					args[32];
	t_run					run;
	size_t					i;

	i = 0;
	while (i < sizeof timings / sizeof timings[0])
	{
		snprintf(args, sizeof args, "2 %ld %ld %ld",
			timings[i].die, timings[i].eat, timings[i].sleep);
		scenario(args, "one philosopher dies, announced within 10ms");
		run = philo(args, 5000);
		check_output_is_well_formed(&run, 2);
		CHECK_MSG(!run.timed_out, "%s: still running, no death was detected", run.label);
		CHECK_MSG(run.exit_code == 0, "%s: exit code %d", run.label, run.exit_code);
		check_death_is_announced_on_time(&run, timings[i].die);
		free_run(&run);
		i++;
	}
}

int	main(int argc, char **argv)
{
	if (argc > 1)
		g_binary = argv[1];
	if (access(g_binary, X_OK) != 0)
	{
		fprintf(stderr, "%s is not built (run make first)\n", g_binary);
		return (2);
	}
	set_colors();
	printf("%sphilo scenario tests%s\n", g_bold, g_reset);
	철학자_1명_먹지_못하고_죽기();
	철학자_5명_아무도_죽지_않기();
	철학자_5명_모두_7번_먹으면_멈추기();
	철학자_4명_빠듯한_일정에서_아무도_죽지_않기();
	철학자_4명_중_한_명_죽기();
	철학자_2명_사망_10ms_안에_출력하기();
	printf("%sphilo scenario tests: %d passed, %d failed (%d checks)%s\n",
		g_failed ? g_red : g_green, g_tests - g_failed, g_failed, g_checked, g_reset);
	return (g_failed != 0);
}
