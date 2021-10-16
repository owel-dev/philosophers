#include "philo_bonus.h"

int	ft_isdigit(int c)
{
	return ('0' <= c && c <= '9');
}

void	kill_all_process(t_info *info)
{
	int	i;

	i = 0;
	while (i < info->philo_count)
		kill(info->philos[i++]->pid, SIGKILL);
}

void	receive_exit(t_info *info)
{
	int	i;
	int	status;
	int	eat_count;

	i = 0;
	eat_count = 0;
	while (i < info->philo_count)
	{
		waitpid(-1, &status, 0);
		if (status == 256)
		{
			kill_all_process(info);
			return ;
		}
		i++;
	}
}

void	eat_monitor(void *arg)

{
	t_info	*info;
	int		i;

	info = (t_info *)arg;
	i = 0;
	while (i < info->philo_count)
	{
		sem_wait(info->all_eat_count);
		i++;
	}
	sem_wait(info->print);
	kill_all_process(info);
}
