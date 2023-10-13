/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mehdimirzaie <mehdimirzaie@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/11 11:07:14 by mmirzaie          #+#    #+#             */
/*   Updated: 2023/10/13 10:57:46 by clovell          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execute.h"
#include "shell.h"
#include <stdio.h>
#include <sys/wait.h>

static bool	is_last_cmd(int num_cmds)
{
	if (num_cmds == 1)
		return (true);
	return (false);
}

static void	open_and_redirect(char *name, int rw, int permission)
{
	int	file_fd;

	file_fd = 0;
	if (!permission)
		file_fd = open(name, rw, 0644);
	else
		file_fd = open(name, rw, permission);
	if (file_fd < 0)
		error_exit(ft_strfmt("%s: %s", name, strerror(errno)), EXIT_FAILURE);
	if (!permission)
	{
		if (redirect(file_fd, STDIN_FILENO) < 0)
			error_exit(ft_strfmt("%s: %s", name,
					strerror(errno)), EXIT_FAILURE);
	}
	else
		if (redirect(file_fd, STDOUT_FILENO) < 0)
			error_exit(ft_strfmt("%s: %s", name,
					strerror(errno)), EXIT_FAILURE);
}

void	handle_cmdredirect(t_ast *ast, t_iolst *redirects,
	int pipe1[2], int num_cmds)
{
	const int	in = dup(STDIN_FILENO);
	t_iolst		*start;

	start = redirects;
	while (start)
	{
		if (start && start->type == E_TTLLA)
		{
			dup2(in, STDIN_FILENO);
			handle_heredoc(start);
		}
		start = start->next;
	}
	(void)ast;
	while (redirects)
	{
		if (!redirects)
			break ;
		if (redirects && redirects->type == E_TTLA)
			open_and_redirect(redirects->str, O_RDONLY, 0);
		if (redirects && redirects->type == E_TTRA)
			open_and_redirect(redirects->str, E_OUT, 0644);
		else if (redirects && redirects->type == E_TTRRA)
			open_and_redirect(redirects->str, E_APPEND, 0644);
		else if (!is_last_cmd(num_cmds))
			redirect(pipe1[1], STDOUT_FILENO);
		else if (pipe1)
			close(pipe1[1]);
		redirects = redirects->next;
	}
}

void	open_file(t_ast *ast, int pipe1[2], int num_cmds)
{
	t_iolst	*redirects;

	redirects = ast->cmd->redirects;
	if (redirects == NULL)
	{
		if (is_last_cmd(num_cmds))
			return ;
		if (pipe1)
			redirect(pipe1[1], STDOUT_FILENO);
	}
	handle_cmdredirect(ast, redirects, pipe1, num_cmds);
}

void	execute(t_ast *ast, t_env **our_env, int *exit_status, int num_cmds)
{
	const int	in = dup(STDIN_FILENO);
	int			pipe1[2];
	pid_t		child;
	t_ast		*node;
	t_iolst		*start;

	while (num_cmds > 0)
	{
		node = get_next_node(ast, num_cmds);
		start = node->cmd->redirects;
		while (start)
		{
			if (start->type == E_TTLLA)
			{
				dup2(in, STDIN_FILENO);
				break ;
			}
			start = start->next;
		}
		if (pipe(pipe1) < 0)
			perror("error making pipe\n");
		get_command_name(&node->cmd->cmd, node->cmd, true);
		child = fork();
		if (child == 0)
		{
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			close(pipe1[0]);
			open_file(node, pipe1, num_cmds);
			if (!node->cmd->cmd)
				exit(EXIT_SUCCESS);
			if (is_builtin(node->cmd) || is_envbuiltin(node->cmd))
				execute_builtin_cmds(node->cmd, our_env, exit_status);
			else
				execute_system_cmds(node->cmd, *our_env);
			exit(EXIT_SUCCESS);
		}
		redirect(pipe1[0], STDIN_FILENO);
		close(pipe1[1]);
		num_cmds--;
		start = node->cmd->redirects;
		while (start)
		{
			if (start->type == E_TTLLA)
			{
				waitpid(child, NULL, 0);
				break ;
			}
			start = start->next;
		}
	}
}
