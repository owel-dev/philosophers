#ifndef PHILO_H
# define PHILO_H
# include <unistd.h>
# include <stdlib.h>
# include <pthread.h>
# include <string.h>
# include <sys/time.h>
# include <stdio.h>

typedef struct s_philo
{
	pthread_t		philo_id;
	int				philo_num;
	long long		when_start;
	long long		when_last_eat;
	int				left_fork;
	int				right_fork;
	int				eat_count;
	struct s_info	*info;
}				t_philo;

typedef struct s_info
{
	t_philo				**philos;
	pthread_t			moniter_id;
	pthread_mutex_t		*fork;
	pthread_mutex_t		print;
	int					philo_count;
	int					die_long;
	int					eat_long;
	int					sleep_long;
	int					must_eat;
	int					infinite;
	int					death;
}				t_info;

int			set_info(t_info *info, int ac, char **av);
int			set_fork(t_info *info);
int			set_philos(t_info *info);
int			make_thread(t_info *info);
void		wait_threads(t_info *info);
void		clean_info(t_info *info);

void		monitor_run(void *arg);
void		philo_run(void *arg);
void		eat(t_philo *philo);
void		sleepy(t_philo *philo);
void		think(t_philo *philo);

int			ft_isdigit(int c);
int			is_space(int c);
int			ft_atoi(const char *str);
long long	get_ms_time(void);
void		print_status(t_philo *philo, char *str);

int			ft_isspace(int c);
int			is_num(char *str);
int			right_range(char *str);
int			check_arg(int ac, char **av);

#endif
