#include "philo_bonus.h"

int	set_info(t_info *info, int ac, char **av)
{
	info->philo_count = ft_atoi(av[1]);
	info->die_long = ft_atoi(av[2]);
	info->eat_long = ft_atoi(av[3]);
	info->sleep_long = ft_atoi(av[4]);
	info->must_eat = -1;
	info->infinite = 1;
	if (ac == 6)
	{
		info->infinite = 0;
		info->must_eat = ft_atoi(av[5]);
	}
	info->death = 0;
	return (0);
}

int	set_sem(t_info *info)
{
	int	i;

	sem_unlink("fork");
	info->fork = sem_open("fork", O_CREAT, 0644, info->philo_count);
	if (info->fork == SEM_FAILED)
		return (-1);
	sem_unlink("print");
	info->print = sem_open("print", O_CREAT, 0644, 1);
	if (info->print == SEM_FAILED)
		return (-1);
	sem_unlink("all_eat_count");
	info->all_eat_count = sem_open("all_eat_count", O_CREAT, 0644, \
		info->philo_count);
	if (info->all_eat_count == SEM_FAILED)
		return (-1);
	i = 0;
	while (i < info->philo_count)
	{
		sem_wait(info->all_eat_count);
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
		info->philos[i] = (t_philo *)malloc(sizeof(t_philo));
		if (info->philos[i] == NULL)
			return (-1);
		info->philos[i]->when_last_eat = time;
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

int	make_process(t_info *info)
{
	int	i;

	i = 0;
	if (pthread_create(&info->moniter_id, NULL, \
		(void *)&eat_monitor, (void *)info) != 0)
		return (-1);
	while (i < info->philo_count)
	{
		info->philos[i]->pid = fork();
		if (info->philos[i]->pid == 0)
		{
			if (pthread_create(&info->moniter_id, NULL, \
				(void *)&monitor_run, (void *)info->philos[i]) != 0)
				return (-1);
			philo_run(info->philos[i]);
			exit(0);
		}
		i++;
		usleep(100);
	}
	receive_exit(info);
	return (0);
}

int	main(int ac, char **av)
{
	t_info	info;

	if (check_arg(ac, av) == -1)
		return (-1);
	if (set_info(&info, ac, av) == -1)
		return (-1);
	if (set_sem(&info) == -1)
		return (-1);
	if (set_philos(&info) == -1)
		return (-1);
	if (make_process(&info) == -1)
		return (-1);
	return (0);
}
