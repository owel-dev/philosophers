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
