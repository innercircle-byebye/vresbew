/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bipipe.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sucho <sucho@student.42seoul.kr>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/07/25 08:45:32 by sucho             #+#    #+#             */
/*   Updated: 2021/07/25 19:48:00 by sucho            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define READ 0
#define WRITE 1
void perror_and_exit(std::string error) {
  perror(error.c_str());
  exit(EXIT_FAILURE);
}

int main() {
  int pipe_stdin[2];
  int pipe_stdout[2];
  int pid;
  char buf;

  if (pipe(pipe_stdin) < 0)
    perror_and_exit("pipe");
  if (pipe(pipe_stdout) < 0)
    perror_and_exit("pipe");
  if ((pid = fork()) < 0)
    perror_and_exit("fork");
  else if (pid == 0) {
    close(pipe_stdout[READ]);
    if (dup2(pipe_stdout[WRITE], STDOUT_FILENO) < 0)
      perror_and_exit("pipe");
    if (execlp("echo", "echo", "testisgood", NULL) < 0)
      perror_and_exit("execlp");
  } else {
    close(pipe_stdout[WRITE]);
    while (read(pipe_stdout[READ], &buf, 1) > 0)
      write(pipe_stdin[WRITE], &buf, 1);
    close(pipe_stdout[READ]);
  }
  if ((pid = fork()) < 0)
    perror_and_exit("fork");
  else if (pid == 0) {
    close(pipe_stdin[WRITE]);
    if (dup2(pipe_stdin[READ], STDIN_FILENO) < 0)
      perror_and_exit("pipe");
    if (execlp("cat", "cat", NULL) < 0)
      perror_and_exit("execlp");
  } else {
    close(pipe_stdin[WRITE]);
    close(pipe_stdin[READ]);
  }
  return (EXIT_SUCCESS);
}
