//handle(exit) for fatal error
void error_handling(char* message);

void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
