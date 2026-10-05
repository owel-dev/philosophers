#include "philo.h"

int	set_info(t_info *info, int ac, char **av)
{
	info->philo_count = ft_atoi(av[1]);
	info->die_long = ft_atoi(av[2]);
	info->eat_long = ft_atoi(av[3]);
	info->sleep_long = ft_atoi(av[4]);
	info->must_eat = 1;
	info->infinite = 1;
	if (ac == 6)
	{
		info->infinite = 0;
		info->must_eat = ft_atoi(av[5]);
	}
	info->death = 0;
	if (pthread_mutex_init(&info->print, 0) != 0)
		return (-1);
	return (0);
}

int	set_fork(t_info *info)
{
	pthread_mutex_t	*fork;
	int				i;

	i = 0;
	info->fork = (pthread_mutex_t *) \
		malloc(sizeof(pthread_mutex_t) * info->philo_count);
	if (info->fork == NULL)
		return (-1);
	while (i < info->philo_count)
	{
		if (pthread_mutex_init(&info->fork[i], 0) != 0)
			return (-1);
		i++;
	}
	return (0);
}

int	set_philos(t_info *info)
{
	int			i;
	long long	time;

	info->philos = (t_philo **)malloc(sizeof(t_philo *) * info->philo_count);
	if (info->philos == NULL)
		return (-1);
	i = 0;
	time = get_ms_time();
	while (i < info->philo_count)
	{
		info->philos[i] = (t_philo *)malloc(sizeof(t_philo) * \
			info->philo_count);
		if (info->philos[i] == NULL)
			return (-1);
		info->philos[i]->when_last_eat = get_ms_time();
		info->philos[i]->when_start = time;
		info->philos[i]->philo_num = i + 1;
		info->philos[i]->info = info;
		info->philos[i]->left_fork = i;
		info->philos[i]->right_fork = (i + 1) % info->philo_count;
		info->philos[i]->eat_count = 0;
		i++;
	}
	return (0);
}

int	make_thread(t_info *info)
{
	int	i;

	i = 0;
	while (i < info->philo_count)
	{
		if (pthread_create(&info->philos[i]->philo_id, NULL, \
			(void *)&philo_run, (void *)info->philos[i]) != 0)
			return (-1);
		usleep(100);
		i++;
	}
	if (pthread_create(&info->moniter_id, NULL, \
		(void *)&monitor_run, (void *)info) != 0)
		return (-1);
	return (0);
}

int	main(int ac, char **av)
{
	t_info	info;

	if (check_arg(ac, av) == -1)
		return (-1);
	if (set_info(&info, ac, av) == -1)
		return (-1);
	if (set_fork(&info) == -1)
		return (-1);
	if (set_philos(&info) == -1)
		return (-1);
	if (make_thread(&info) == -1)
		return (-1);
	wait_threads(&info);
	clean_info(&info);
	return (0);
}
