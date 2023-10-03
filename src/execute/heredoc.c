/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mehdimirzaie <mehdimirzaie@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/25 17:54:48 by mehdimirzai       #+#    #+#             */
/*   Updated: 2023/10/04 09:06:41 by mehdimirzai      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "execute.h"

/*
fix heredoc.
*/

void	error_exit(char *error, int val)
{
	ft_putstr_fd(error, 2);
	ft_putchar_fd('\n', 2);
	exit(val);
}

void	readheredoc(t_ast *ast, int fd, const char *filename)
{
	char		*lines;

	lines = readline("heredoc> ");
	while (ft_strcmp(lines, ast->u_node.cmd->redirects->str) != 0)
	{
		ft_putstr_fd_nl(lines, fd, true);
		free(lines);
		lines = readline("heredoc> ");
		if (!lines || g_value == SIGUSR1)
		{
			if (unlink(filename) == -1)
				perror("unlink");
			reset_termios();
			exit(130);
		}
	}
}

void	handle_heredoc(t_ast *ast)
{
	const char	*filename = "/tmp/mytempfileXXXXXX";
	int			fd;

	fd = open(filename, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1)
		error_exit("error making temp file for heredoc", 127);
	init_termios();
	readheredoc(ast, fd, filename);
	reset_termios();
	close(fd);
	fd = open(filename, O_RDONLY);
	redirect(fd, STDIN_FILENO);
	if (unlink(filename) == -1)
		perror("unlink");
}
