#include "philo.h"

void	monitor_run(void *arg)
{
	t_info		*info;
	int			i;
	int			all_must_eat;

	info = (t_info *)arg;
	while (info->death == 0)
	{
		i = 0;
		all_must_eat = 1;
		while (i < info->philo_count)
		{
			if (get_ms_time() - info->philos[i]->when_last_eat >= \
				info->die_long)
			{
				print_status(info->philos[i], "died");
				info->death = 1;
			}
			if (info->philos[i]->eat_count < info->must_eat)
				all_must_eat = 0;
			i++;
			usleep(500);
		}
		if (all_must_eat == 1)
			info->death = 1;
	}
}

void	philo_run(void *arg)
{
	t_philo	*philo;

	philo = (t_philo *)arg;
	while (philo->info->death == 0)
	{
		eat(philo);
		sleepy(philo);
		think(philo);
		usleep(100);
	}
}

void	eat(t_philo *philo)
{
	pthread_mutex_lock(&philo->info->fork[philo->left_fork]);
	print_status(philo, "has taken a fork");
	if (philo->info->philo_count == 1)
	{
		while (philo->info->death == 0)
			usleep(500);
		pthread_mutex_unlock(&philo->info->fork[philo->left_fork]);
		return ;
	}
	pthread_mutex_lock(&philo->info->fork[philo->right_fork]);
	print_status(philo, "has taken a fork");
	philo->when_last_eat = get_ms_time();
	print_status(philo, "is eating");
	while (get_ms_time() - philo->when_last_eat < philo->info->eat_long)
		usleep(600);
	if (philo->info->infinite == 0)
		philo->eat_count++;
	pthread_mutex_unlock(&philo->info->fork[philo->right_fork]);
	pthread_mutex_unlock(&philo->info->fork[philo->left_fork]);
}

void	sleepy(t_philo *philo)
{
	long long	time;

	time = get_ms_time();
	print_status(philo, "is sleeping");
	while (get_ms_time() - time < philo->info->sleep_long)
		usleep(600);
}

void	think(t_philo *philo)
{
	print_status(philo, "is thinking");
}
