
 #include<iostream>
 #include<sys/socket.h>
 #include<arpa/inet.h>
 #include<csignal>
 #include<sys/wait.h>
 #include<netdb.h>
 #include<cstring>
 #include<string>
 #include<cerrno>
 #include<ctime>
 #include<sstream>
 #include<fstream>
 #include<fcntl.h>
 #include<unistd.h>
#include<cstdlib>
 using namespace std;

 int connfd , cpid , listenfd ;
 sockaddr_in client ;
 socklen_t len ;
 char *cli_hname ;
 FILE *fin , *fout ;
 int ind , outd , did , ccpid ;
 bool done ;
 string resulta ;

 void display(string);

 char *reverse_resolve(in_addr client)
{
 hostent *host = gethostbyaddr((const void *)&client,4,AF_INET) ;
 return ( host == NULL ) ? NULL : (host->h_name) ;
}

 void c_handle(int sig)
{
 close(connfd) ;
 close(ind) ;
 close(outd) ;
 display("gdialog --msgbox \"Call terminated\"") ;
 exit(0) ;
}

 void handle(int sig)
{
 int p ;
 ostringstream dialog ;
 switch( sig )
 {
	case SIGINT :
		close(connfd) ;
		close(ind) ;
		close(outd) ;
		break ;
	case SIGPIPE :
		close(ind) ;
		close(outd) ;
		close(connfd) ;
		dialog << "gdialog --msgbox " ;
		if( cli_hname == NULL )
			dialog << "\"Call terminated by " << inet_ntoa(client.sin_addr) <<  "\"" ;
		else
			dialog << "\"Call terminated by " << cli_hname << "\"" ;
		display(dialog.str()) ;
		break ;
	case SIGTERM :
		close(connfd) ;
		close(ind) ;
		close(outd) ;
		kill(cpid,SIGINT) ;
		break ;
	case SIGCHLD :
		p = waitpid(-1,NULL,0) ;
		if( p == ccpid )
		{
			close(connfd) ;
			close(ind) ;
			close(outd) ;
			kill(cpid,SIGINT) ;
		}
		break ;
	case SIGALRM :
		if( done == 0 )
		{
			close(connfd) ;
			kill(did,SIGKILL) ;
			resulta = "REJECTED" ;
		}
		break ;
 }
 return ;
}

 bool is_ipaddr(char *p)
{
 if( inet_addr(p) == -1 )
 {
	cout << "Bind_address specified " << p << " is not a valid IPv4 address\nValid arguments are : chat_serv <port> or chat_serv <bind_addr> <port>\n" ;
	exit(1) ;
 }
 return 1 ;
}

 bool isint(char *p)
{
 for( int i=0 ; i<strlen(p)-1 ; i++ )
 {
	if( !isdigit(p[i]) )
	{
		cout << "Invalid port to listen on \nValid arguments are : chat_serv <port> or chat_serv <bind_addr> <port>\n" ;
		exit(1) ;
	}
 }
 return 1 ;
}

 void display(string line)
{
 if( (did = fork()) == 0 )
 {
	close(connfd) ;
	close(ind) ;
	close(outd) ;
	close(listenfd) ;
	execlp("sh","sh","-c",line.c_str(),NULL) ;
	exit(1) ;
 }
 signal(SIGCHLD,SIG_IGN) ;
 wait(NULL) ;
 signal(SIGCHLD,handle) ;
 return ;
}

 int main(int argc,char **argv)
{
 int f = fork() ;
 if( f < 0 )
	perror("") , exit(1) ;
 if( f > 0 )
	exit(0) ;
 ifstream finput(argv[1]) ;
 if( argc != 2 )
	cerr << "Usage : " << argv[0] << " confFile\n" , exit(1) ;
 if( !finput )
	cerr << "Error reading from specified configuration file " << argv[1] << " : " << strerror(errno) << "\n" , exit(1) ;
 string recorder("arecord") ;
 string player("aplay") ;
 string binding_addr("0.0.0.0") ;
 string binding_port("1235") ;
 string line ;
 while( getline(finput,line) )
 {
	if( line.size() == 0 )
		;
	else if( line[0] == '#' )
		;
	else
	{
		int t = line.find("#",0) ;
		if( t != -1 )
			line = line.substr(0,t) ;
		int br = line.find("=",0) ;
		if( br != -1 )
		{
			string option = line.substr(0,br) ;
			string value = line.substr(br+1,line.size()-br) ;
			if( option == "RECORDER" )
				recorder = value ;
			else if( option == "PLAYER" )
				player = value ;
			else if( option == "BIND_ADDR" )
				binding_addr = value ;
			else if( option == "BIND_PORT" )
				binding_port = value ;
		}
	}
 }
 string bind_addr = binding_addr ;
 int bind_port = atoi(binding_port.c_str()) ;
 sockaddr_in server ;
 bzero(&server,sizeof(server)) ;
 server.sin_family = AF_INET ;
 server.sin_port = htons(bind_port) ;
 server.sin_addr.s_addr = INADDR_ANY ;
 if( (listenfd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
	perror("Error creating end point on server") , exit(1) ;
 if( bind(listenfd,(sockaddr *)&server,sizeof(server)) < 0 )
	cerr << "Binding on " << bind_addr << ":" << bind_port << " failed : " << strerror(errno) << "\n" , exit(1) ;
 if( listen(listenfd,2) < 0 )
	cerr << "Listening on " << bind_addr << ":" << bind_port << " failed : " << strerror(errno) << "\n" , exit(1) ;
 len = sizeof(client) ;
 cout << "Server started and listening on " << bind_addr << ":" << bind_port << "\n" ;
 for( ; ; )
 {
	begin:
	resulta = "ACCEPTED" ;
	connfd = accept(listenfd,(sockaddr *)&client,&len) ;
	in_addr cli_ip = client.sin_addr ;
	ostringstream dialog ;
	dialog << "gdialog --msgbox " ;
 	if( (cli_hname = reverse_resolve(cli_ip)) == NULL )
		dialog << "\"Call from " << inet_ntoa(cli_ip) << " (Unknown Host)\"" ;
	else
		dialog << "\"Call from " << cli_hname << "(" << inet_ntoa(cli_ip) << ")\"" ;
	done = 0 ;
	signal(SIGALRM,handle) ;
	alarm(55) ;
	display(dialog.str()) ;
	string sync("ACCEPTED") ;
	string async("REJECTED") ;
	done = 1 ;
	if( resulta == "REJECTED" )
		goto begin ;
 	write(connfd,sync.c_str(),8) ;
	recorder += " 2>/dev/null" ;
	player += " 2>/dev/null" ;
	fin = popen(recorder.c_str(),"r") ;
	fout = popen(player.c_str(),"w") ;
	outd = fileno(fout) ;
	ind = fileno(fin) ;
	int n ;
	string line ;
	if( (cpid = fork()) == 0 )
	{
		close(listenfd);
		signal(SIGINT,c_handle) ;
		if( fout == NULL )
        	{
                	perror("Error opening sound recorder") ;
	                close(connfd);
        	        cerr << "Call ended prematurely\n" ;
			exit(0) ;
                	goto begin;
        	}
		while( (n = read(connfd,(char *)line.c_str(),1200)) > 0 )
			if( write(outd,line.c_str(),n) != n )
				exit(1) ;
		exit(0) ;
	}
	else
	{
		signal(SIGCHLD,handle) ;
		if( (ccpid = fork()) == 0 )
		{
			close(listenfd) ;
			close(connfd) ;
			close(ind) ;
			close(outd) ;
			display("gdialog --msgbox \"Click OK to terminate call\"") ;
			exit(0) ;
		}
		signal(SIGINT,handle) ;
		signal(SIGTERM,handle) ;
		signal(SIGPIPE,handle) ;
		if( fin == NULL )
        	{
                	perror("Error opening sound recorder") ;
	                close(connfd);
        	        cerr << "Call ended prematurely\n" ;
                	goto begin;
        	}
		while( (n = read(ind,(char *)line.c_str(),1200)) > 0 )
			write(connfd,line.c_str(),n) ;
	}
 }
 return 0;
}
