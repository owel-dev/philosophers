#include "philo_bonus.h"

int	ft_isspace(int c)
{
	if ((9 <= c && c <= 13) || c == ' ')
		return (1);
	return (0);
}

int	is_num(char *str)
{
	int	i;

	i = 0;
	if (str == NULL)
		return (0);
	while (str[i] != '\0')
	{
		if (ft_isdigit(str[i]) == 0)
			return (0);
		i++;
	}
	return (1);
}

int	ft_atoi(const char *str)
{
	long long	num;
	int			is_minus;

	is_minus = 0;
	while (ft_isspace(*str))
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

int	right_range(char *str)
{
	long long	num;
	int			is_minus;

	is_minus = 0;
	while (ft_isspace(*str))
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
			return (0);
		if (num > 2147483647 && is_minus == 0)
			return (0);
		str++;
	}
	return (1);
}

int	check_arg(int ac, char **av)
{
	int	i;

	if (ac < 5 || ac > 6)
		return (-1);
	i = 1;
	while (i < ac)
	{
		if (av[i][0] == '-' || is_num(av[i]) == 0 || right_range(av[i]) == 0)
			return (-1);
		i++;
	}
	return (0);
}
