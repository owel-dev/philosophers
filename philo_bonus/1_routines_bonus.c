#include "philo_bonus.h"

void	monitor_run(void *arg)
{
	t_philo		*philo;
	int			i;

	philo = (t_philo *)arg;
	while (philo->info->death == 0)
	{
		if (get_ms_time() - philo->when_last_eat >= philo->info->die_long)
		{
			philo->info->death = 1;
			print_status(philo->info->philos[i], "died");
			exit(1);
		}
		usleep(100);
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
	sem_wait(philo->info->fork);
	sem_wait(philo->info->fork);
	philo->when_last_eat = get_ms_time();
	print_status(philo, "has taken a fork");
	print_status(philo, "is eating");
	philo->eat_count++;
	while (get_ms_time() - philo->when_last_eat < philo->info->eat_long)
		usleep(600);
	if (philo->eat_count == philo->info->must_eat)
		sem_post(philo->info->all_eat_count);
	sem_post(philo->info->fork);
	sem_post(philo->info->fork);
}

void	sleepy(t_philo *philo)
{
	long long	time;

	time = get_ms_time();
	print_status(philo, "is sleeping");
	while (get_ms_time() - time < philo->info->sleep_long)
		usleep(500);
}

void	think(t_philo *philo)
{
	print_status(philo, "is thinking");
}
