/*Командный интерпретатор.

Необходимо реализовать под управлением ОС Unix интерактивный командный
интерпретатор (некоторый аналог shell), осуществляющий в цикле считывание командной
строки со стандартного ввода, ее анализ и исполнение соответствующих действий.
В командной строке могут присутствовать следующие операции:
указаны в порядке убывания приоритетов (на одной строке приоритет одинаков)

	1)| , > , >> , <
	2)&& , ||
	3); , &

Допустимы также круглые скобки, которые позволяют изменить порядок выполнения
операций. Для выполнения команды в скобках создаётся отдельный экземпляр Shellа.
В командной строке допустимо также произвольное количество пробелов между
составляющими ее словами.

Разбор командной строки осуществляется Shelloм по следующим правилам:

	+*<Команда Shellа> → <Команда с условным выполнением>{ [; | &] <Команда Shellа>}{;|&}
	+*<Команда с условным выполнением> → <Команда> {[&& | || ] <Команда с условным выполнением>}
	+*<Команда> → {<перенаправление ввода/вывода>}<Конвейер> | <Конвейер>{<перенаправление ввода/вывода>} | ( <Команда Shellа>)
	+*<перенаправление ввода/вывода> → {<перенаправление ввода > } <перенаправление вывода> | {<перенаправление вывода> 	
<перенаправление ввода >
	+*<перенаправление ввода > → ‘<’ файл
	+*<перенаправление вывода> → ‘>’ файл | ‘>>’ файл
	+*<Конвейер> → <Простая команда> {‘|’ <Конвейер>}
	+*<Простая команда> → <имя команды><список аргументов>
	
	{X} – означает, что X может отсутствовать;
	[x|y] – значит, что должен присутствовать один из вариантов : x либо y
	| - в описании правил то же, что «ИЛИ»
	pr1 | ...| prN – конвейер: стандартный вывод всех команд, кроме последней, направляется на стандартный ввод следующей 
команды конвейера. Каждая команда выполняется как самостоятельный процесс (т.е. все pri выполняются параллельно). Shell ожидает
завершения последней команды. Код завершения конвейера = код завершения последней команды конвейера.
Простую команду можно рассматривать как частный случай конвейера.

	com1 ; com2 – означает, что команды будут выполняться последовательно
	com & - запуск команды в фоновом режиме (т.е. Shell готов к вводу следующей команды, не ожидая завершения данной команды 
com, а com не реагирует на сигналы завершения, посылаемые с клавиатуры, например, на нажатие Ctrl-C ). После завершения 
выполнения фоновой команды не должно остаться процесса – зомби. Посмотреть список работающих процессов можно с помощью команды 
ps.
	com1 && com2 - выполнить com1, если она завершилась успешно, выполнить com2;
	com1 || com2 - выполнить com1, если она 
завершилась неуспешно, выполнить com2. Должен быть проверен и системный успех и значение, возвращенное exit ( 0 – успех).

Перенаправление ввода-вывода:
	< файл - файл используется в качестве стандартного ввода;
	> файл - стандартный вывод направляется в файл (если файла не было - он создается, если файл уже существовал, то его старое 
содержимое отбрасывается, т.е. происходит вывод с перезаписью);
	>> файл – стандартный вывод направляется в файл (если файла не было - он создается, если файл уже существовал, то его 
старое содержимое сохраняется, а запись производится в конец файла)

Замечание. В приведенных правилах указаны все возможные способы размещения команд
перенаправления ввода/вывода в командной строке, допустимые стандартом POSIX.
Shell, как правило, поддерживает лишь какую-то часть из них. Для реализации можно
выбрать любой (один) вариант размещения.

Обязательный минимум (достаточный для получения 3) – это реализация конвейера, фонового режима и перенаправлений ввода-вывода.*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define Max_Len 1000
#define Max_Count 30
int shell_commands(char **args, int start, int end);

void del_blank(char* str, char* new_str){
	int j = 0, i = 0;
	while (i < strlen(str)){
		if (str[i] == ' '){
			new_str[j] = str[i];
			j++;
			while ((str[i] == ' ') && (i < strlen(str)))
				i++;
		}
		else{
			new_str[j] = str[i];
			j++;
			i++;
		}
	}	
	new_str[j - 1] = '\0';
}

int get_arguments(char* args[], char* new_line){
	int i = 0;
	int count = 0;
	while (new_line[i] != '\0'){
		if (new_line[i] == ' ')
			i++;
		char* arg_str = (char*) malloc(sizeof(char)*8);
		int len = 8;
		int j = 0;
		while ((new_line[i] != '\0') && (new_line[i] != ' ')){
			arg_str[j] = new_line[i];
			j++;
			i++;
			if ( j % 8 == 0){
				len += 8;
				arg_str = (char *)realloc(arg_str, len);
			}
		}
		if ( j % 8 == 0){
				arg_str = (char *)realloc(arg_str, len + 1);
		}
		arg_str[j] = '\0';
		args[count] = arg_str;
		count++;
	}
	return count;
}

int brackets(char **args, int start, int end){
	int count = 0;
	for (int i = start; i < end; i++){
		if (strcmp(args[i], "(" )== 0)
			count ++;
		if (strcmp(args[i], ")" )== 0)
			count--;
	}
	if (count == 0)
		return 0;
	else
		return -1;
}

int redirection (char **args, int start, int end, int *arg_st, int *in_file, int *out_file, int *count){
	//printf("redirection в разработке\n");
	int i = 0;
	int new_start = start, new_end = end;
	int dir_file = 0;
	
	for (int j = 0; j < 2; j++)
		if ((args[i][0] == '<') || (args[i][0] == '>'))
			i += 2;
	int j = start;
	while ((i + j < end) && (args[i][0] != '<') && (args[i][0] != '>')){
		if (args[i][0] == '|')
			(*count)++;
		i++;
	}
	(*count)++;
	if (i + j >= end)
		i = 0;
	for (j = 0; j < 2; j++){
		if ((i < end) && ((args[i][0] == '<') || (args[i][0] == '>'))){
			if (strcmp(args[new_start + 2*j], "<") == 0){
				if ((*in_file = open(args[new_start + 1 + 2*j], O_RDONLY, 0666)) == -1){
					printf ("File open error\n");
					return -1;
				}
				dir_file += 2;
				(*arg_st) += 2;
			}
			else if (strcmp(args[new_end - 2 - 2*j], "<") == 0){
				if ((*in_file = open(args[new_end - 1 - 2*j], O_RDONLY, 0666)) == -1){
					printf ("File open error\n");
					return -1;
				}
				dir_file += 2;
			}
			else if (strcmp(args[new_start + 2*j], ">") == 0){
				if ((*out_file = open(args[new_start + 1 + 2*j], O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1){
					printf ("File open error\n");
					return -1;
				}
				dir_file += 1;
				(*arg_st) += 2;
			}
			else if (strcmp(args[new_end - 2 - 2*j], ">") == 0){
				if ((*out_file = open(args[new_end - 1 - 2*j], O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1){
					printf ("File open error\n");
					return -1;
				}
				dir_file += 1;
			}
			else if (strcmp(args[new_start + 2*j], ">>") == 0){
				if ((*out_file = open(args[new_start + 1 + 2*j], O_WRONLY | O_APPEND, 0666)) == -1){
					if ((*out_file = open(args[new_start + 1 + 2*j], O_WRONLY | O_CREAT, 0666)) == -1){
						printf ("File open error\n");
						return -1;
					}
				}
				dir_file += 1;
				(*arg_st) += 2;
			}
			else if (strcmp(args[new_end - 2 - 2*j], ">>") == 0){
				if ((*out_file = open(args[new_end - 1 - 2*j], O_WRONLY | O_APPEND, 0666)) == -1){
					if ((*out_file = open(args[new_end - 1 - 2*j], O_WRONLY | O_CREAT, 0666)) == -1){
						printf ("File open error\n");
						return -1;
					}
				}
				dir_file += 1;
			}
			i += 2;
		}
	}
	return dir_file;
}

int transporter (char **args, int start, int end){
	//printf("transporter в разработке\n");
	int count = 0, status, fd[2], prev;
	int in_file, out_file, arg_st = 0, len;
	int result;
	if ((result = redirection(args, start, end, &arg_st, &in_file, &out_file, &count))== -1)
		return 1;
	pid_t pids[count + 1];
	for (int i = 0; i < count; i++)
	{
		len = 0;
		while ((start + arg_st + len < end) && (strcmp(args[arg_st + len], "|") != 0) && (strcmp(args[arg_st + len], ">") != 0) && (strcmp(args[arg_st + len], "<") != 0) && (strcmp(args[arg_st + len], "<<") != 0)){
			len ++;
		}
		char *mas[len + 1];
		for (int j = 0; j < len; j++)
		{
			mas[j] = args[arg_st + j];
		}
		mas[len] = NULL;
		pipe(fd);
		if ((pids[i] = fork()) == 0)
		{
			if ((i == 0) && ((result & 2) == 2))
				dup2(in_file, 0);
			else if (i != 0)
				dup2(prev, 0);
			if (i != count - 1)
				dup2(fd[1], 1); 
			else if ((result & 1) == 1)
				dup2(out_file, 1);
			close(fd[0]);
			close(fd[1]);
			execvp(args[arg_st], mas);
			printf("*\n");
			exit(-1);
		}
		close(fd[1]);
		prev = fd[0];
		arg_st = arg_st + 1 + len;
	}
	for (int i = 0; i < count; i++){
		waitpid(pids[i], &status, 0);
	}
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	else {
		printf("Executed error\n");
		return 1;
	}
}

int exec_command(char **args, int start, int end){
	//printf("exec_command в разработке\n");
    int status, res;
	
	if (strcmp(args[start], "(") != 0)
		return (transporter(args, start, end));
    else {
    	pid_t pid;
    	if ((pid = fork()) == -1){
    		printf("Process error\n");
    		return 1;
    	}
    	else if (pid == 0){
    		res = shell_commands(args, start + 1, end);	
    		exit(res);
    	}
    	wait(&status);
    	if (WIFEXITED(status)){
    		return WIFEXITED(status);
    	}
    	else{
    		return 1;
    	}
    }
}

int cond_exec_command(char **args, int start, int end){
	//printf("cond_exec_command в разработке\n");	
    char *command_args[Max_Count];
	int i = 0, j = start, flag;
	int new_start = start;
	int new_end = end;
	if (strcmp(args[new_start], "(") == 0){
		if (brackets(args, j, new_end) == -1){
			flag = 1;
		}
		else{
			while (strcmp(args[j], ")") != 0){
				command_args[i] = args[j];
				i++;
		    	j++;
			}
			flag = exec_command(command_args, 0, i);
			if (j < new_end - 1)
				if (((flag == 1) && (strcmp(args[j + 1], "&&") == 0))|| ((flag == 0) && (strcmp(args[j + 1], "||") == 0)))
					cond_exec_command(args, j + 2, new_end);
		}
	}
	else{
		while ((j < new_end) && (strcmp(args[j], "||") != 0) && (strcmp(args[j], "&&") != 0)){
			command_args[i] = args[j];
			i++;
		    j++;
		}
		//printf("%d\n", j);
		if (j >= new_end){
			exec_command(command_args, 0, i);
		}
		else if (strcmp(args[j], "&&") == 0){ // если выполнилась команда 1
		    if (exec_command(command_args, 0, i) == 0){
		        cond_exec_command(args, j + 1, new_end);
		    }
		}
		else if (strcmp(args[j], "||") == 0){ // если не выполнилась команда 1
		    if (exec_command(command_args, 0, i) == 1){
		        cond_exec_command(args, j + 1, new_end);
		    }
		}
	}
}

void free_args(char **args, int count){
	//printf("free_args в разработке\n");
	int i = 0;
	while (i < count){
		free(args[i]);
		i++;
	}
}

int shell_commands(char **args, int start, int end){
	int i = 0, j = start;
	char *command_args[Max_Count];
	int new_start = start;
	int new_end = end;
	if (brackets(args, start, end) == -1)
		return 1;
	/*if (strcmp(args[j], "(") == 0){
		while (strcmp(args[j++], ")") != 0);
		if ((j + 1 >= end) || ())
		printf("%d|%d\n", start + 1, start + j + 1);
		shell_commands(args, start + 1, start + j - 1);
		j -= 1;
	}
	else{*/
		while ((j < new_end) && (strcmp(args[j], ";") != 0) && (strcmp(args[j], "&") != 0)){
				command_args[i] = args[j];
				i++;
				j++;
		}
		//printf("%d|%d\n", j, new_end);
		if (j >= new_end){
			//printf("*\n");
			cond_exec_command(command_args, 0, i);
		}
		else if (strcmp(args[j], "&") == 0){ // переход в фоновый режим
			if (fork() == 0){
				if (fork() == 0){
					int fd[2];
					fd[0] = open("/dev/null", O_RDONLY);
				    fd[1] = open("/dev/null", O_WRONLY);
				    dup2(fd[0], 0);
				    dup2(fd[1], 1);
				    close(fd[0]);
				    close(fd[1]);
				    signal(SIGINT, SIG_IGN);
					shell_commands(command_args, 0, i); 
					free_args(args, end);
					exit(0);
				}
				else{
					free_args(args, end);
					exit(0);
				}
			}
			wait(NULL);
		}
		else if (strcmp(args[j], ";") == 0){
			cond_exec_command(command_args, 0, i);   //1 - последовательно
		}
		else {
			cond_exec_command(command_args, 0, i);
		}
	new_start = j + 1;
	//printf("%d|%d\n", new_start, new_end);
	if (new_start < new_end){ //если есть команда шелла 
		shell_commands(args, new_start, new_end);
	}	
}


int main(){
	char line[Max_Len + 1]; 
	char *args[Max_Count];
	char new_line[Max_Count];
	while (1){
		int count_args = 0;
		fgets(line, Max_Len, stdin);
		if (strlen(line) == 0){
			continue;
		}
		del_blank(line, new_line);
		//printf("%s\n", new_line);
		count_args = get_arguments(args, new_line);
		//printf("%d\n", count_args);
		shell_commands(args, 0, count_args);
		free_args(args, count_args);
	}	
}
