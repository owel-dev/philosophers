#include "philo.h"

int	ft_isdigit(int c)
{
	return ('0' <= c && c <= '9');
}

int	is_space(int c)
{
	if ((9 <= c && c <= 13) || c == ' ')
		return (1);
	return (0);
}

int	ft_atoi(const char *str)
{
	long long	num;
	int			is_minus;

	is_minus = 0;
	while (is_space(*str))
		str++;
	if (*str == '-')
		is_minus = 1;
	if (*str == '-' || *str == '+')
		str++;
	num = 0;
	while (ft_isdigit(*str))
	{
		num *= 10;
		num += *str - '0';
		if (num > 2147483648 && is_minus == 1)
			return (-1);
		if (num > 2147483647 && is_minus == 0)
			return (-1);
		str++;
	}
	if (is_minus)
		num *= -1;
	return (num);
}

long long	get_ms_time(void)
{
	struct timeval	te;
	long long		ms;

	gettimeofday(&te, NULL);
	ms = te.tv_sec * 1000 + te.tv_usec / 1000;
	return (ms);
}

void	print_status(t_philo *philo, char *str)
{
	pthread_mutex_lock(&philo->info->print);
	if (philo->info->death == 1)
	{
		pthread_mutex_unlock(&philo->info->print);
		return ;
	}
	printf("%lld %d %s\n", get_ms_time() - philo->when_start, \
		philo->philo_num, str);
	pthread_mutex_unlock(&philo->info->print);
}
