#include "philo_bonus.h"

#define CHECK(expr) check((expr), #expr, __LINE__)

static int		g_checked;
static int		g_failed;
static int		g_tests;
static int		g_current_failed;
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

static void	check(int ok, const char *expr, int line)
{
	g_checked++;
	if (!ok)
	{
		if (!g_current_failed)
			g_failed++;
		g_current_failed = 1;
		printf("  %sFAIL%s %s line %d: %s\n", g_red, g_reset, __FILE__, line, expr);
	}
}

static void	describe(const char *function, const char *expectation)
{
	g_tests++;
	g_current_failed = 0;
	printf("  %s%-18s%s %s\n", g_cyan, function, g_reset, expectation);
}

static void	atoi_일반_숫자와_부호_숫자_읽기(void)
{
	describe("ft_atoi", "reads plain and signed numbers");
	CHECK(ft_atoi("0") == 0);
	CHECK(ft_atoi("42") == 42);
	CHECK(ft_atoi("   42") == 42);
	CHECK(ft_atoi("+42") == 42);
	CHECK(ft_atoi("-42") == -42);
	CHECK(ft_atoi("12abc") == 12);
	CHECK(ft_atoi("") == 0);
	CHECK(ft_atoi("2147483647") == 2147483647);
	CHECK(ft_atoi("-2147483648") == -2147483647 - 1);
}

static void	atoi_int_범위_초과_값_표시하기(void)
{
	describe("ft_atoi", "returns -1 for values outside the int range");
	CHECK(ft_atoi("2147483648") == -1);
	CHECK(ft_atoi("-2147483649") == -1);
	CHECK(ft_atoi("99999999999") == -1);
}

static void	is_num_부호_없는_숫자_문자열만_받기(void)
{
	describe("is_num", "accepts only unsigned digit strings");
	CHECK(is_num("5") == 1);
	CHECK(is_num("800") == 1);
	CHECK(is_num("") == 0);
	CHECK(is_num("-5") == 0);
	CHECK(is_num("+5") == 0);
	CHECK(is_num("5a") == 0);
	CHECK(is_num(" 5") == 0);
	CHECK(is_num(NULL) == 0);
}

static void	right_range_int_범위_안의_값만_통과시키기(void)
{
	describe("right_range", "passes only values inside the int range");
	CHECK(right_range("0") == 1);
	CHECK(right_range("2147483647") == 1);
	CHECK(right_range("2147483648") == 0);
	CHECK(right_range("-2147483648") == 1);
	CHECK(right_range("-2147483649") == 0);
}

static void	check_arg_올바른_인자_묶음_받기(void)
{
	char	*five[] = {"./philo_bonus", "5", "800", "200", "200", NULL};
	char	*six[] = {"./philo_bonus", "5", "800", "200", "200", "7", NULL};
	char	*one[] = {"./philo_bonus", "1", "800", "200", "200", NULL};
	char	*zero_times[] = {"./philo_bonus", "5", "800", "200", "200", "0", NULL};

	describe("check_arg", "accepts valid argument sets");
	CHECK(check_arg(5, five) == 0);
	CHECK(check_arg(6, six) == 0);
	CHECK(check_arg(5, one) == 0);
	CHECK(check_arg(6, zero_times) == 0);
}

static void	check_arg_잘못된_개수와_값_거르기(void)
{
	char	*four[] = {"./philo_bonus", "5", "800", "200", NULL};
	char	*seven[] = {"./philo_bonus", "5", "800", "200", "200", "7", "1", NULL};
	char	*negative[] = {"./philo_bonus", "5", "-800", "200", "200", NULL};
	char	*text[] = {"./philo_bonus", "five", "800", "200", "200", NULL};
	char	*empty[] = {"./philo_bonus", "5", "800", "", "200", NULL};
	char	*big[] = {"./philo_bonus", "5", "2147483648", "200", "200", NULL};
	char	*nobody[] = {"./philo_bonus", "0", "800", "200", "200", NULL};

	describe("check_arg", "rejects wrong counts and values");
	CHECK(check_arg(4, four) == -1);
	CHECK(check_arg(7, seven) == -1);
	CHECK(check_arg(5, negative) == -1);
	CHECK(check_arg(5, text) == -1);
	CHECK(check_arg(5, empty) == -1);
	CHECK(check_arg(5, big) == -1);
	CHECK(check_arg(5, nobody) == -1);
}

int	main(void)
{
	set_colors();
	printf("%sphilo_bonus argument parsing%s\n", g_bold, g_reset);
	atoi_일반_숫자와_부호_숫자_읽기();
	atoi_int_범위_초과_값_표시하기();
	is_num_부호_없는_숫자_문자열만_받기();
	right_range_int_범위_안의_값만_통과시키기();
	check_arg_올바른_인자_묶음_받기();
	check_arg_잘못된_개수와_값_거르기();
	printf("%sphilo_bonus argument parsing: %d passed, %d failed (%d checks)%s\n", \
		g_failed ? g_red : g_green, g_tests - g_failed, g_failed, g_checked, g_reset);
	return (g_failed != 0);
}
