#include "philo.h"

void	wait_threads(t_info *info)
{
	int	i;

	i = 0;
	while (i < info->philo_count)
	{
		pthread_join(info->philos[i]->philo_id, NULL);
		i++;
	}
	pthread_join(info->moniter_id, NULL);
}

void	clean_info(t_info *info)
{
	int	i;

	i = 0;
	while (i < info->philo_count)
	{
		pthread_mutex_destroy(&info->fork[i]);
		free(info->philos[i]);
		i++;
	}
	pthread_mutex_destroy(&info->print);
	free(info->fork);
	free(info->philos);
}

void	print_all_ate(t_info *info)
{
	pthread_mutex_lock(&info->print);
	if (info->death == 0)
	{
		info->death = 1;
		printf("%lld all philosophers have eaten %d times\n", \
			get_ms_time() - info->philos[0]->when_start, info->must_eat);
	}
	pthread_mutex_unlock(&info->print);
}
