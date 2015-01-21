#include<cstdlib>
 #include<iostream>
 #include<sys/socket.h>
 #include<arpa/inet.h>
 #include<csignal>
 #include<cerrno>
 #include<string>
 #include<sys/wait.h>
 #include<netdb.h>
 #include<unistd.h>
 #include<fcntl.h>
#include<cstring>
 using namespace std;

 sockaddr_in server ;
 int sockfd,newd ;
 char *hname , *h_ip ;
 FILE *fin , *fout ;
 int ind , outd , cpid ;

 char *resolve(char *name)
{
 hostent *host = gethostbyname(name) ;
 return ( host == NULL ) ? NULL : inet_ntoa( *(in_addr *)host->h_addr ) ;
}

 char *reverse_resolve(char *name)
{
 in_addr server ;
 inet_aton(name,&server) ;
 hostent *host = gethostbyaddr((const void *)&server,4,AF_INET) ;
 return (host == NULL) ? (NULL) : (host->h_name) ;
}

 void handle(int sig)
{
 switch( sig )
 {
 	case SIGINT :
		close(sockfd);
		close(ind);
		cout << "Call terminated\n" ;
		exit(1) ;
	case SIGTERM :
		close(sockfd);
		close(ind);
		break;
	case SIGPIPE :
		kill(SIGINT,cpid) ;
		if( hname != NULL )
			cout << "Call terminated by " << hname << "(" << h_ip << ")\n" ;
		else 
			cout << "Call terminated by " << h_ip << endl ;
		close(ind);
		exit(1) ;
	case SIGCHLD :
		waitpid(-1,NULL,0);
		break;
 }
 return ;
}

 void c_handle(int s)
{
 exit(1) ;
}

 int main(int argc,char **argv)
{
 if( argc != 3 )
	cerr << "Usage : <call> <server_ip/hostname> <tcp_port_num>\n" , exit(1) ;
 if( inet_addr(argv[1]) == -1 )
 {
	h_ip = resolve(argv[1]) ;
	hname = argv[1] ;
 	if( h_ip == NULL )
		cerr << "call : unknown host " << argv[1] << " (If you think this is a bug use your host program to resolve this hostname) \n" , exit(1) ;
 }
 else
 {
	h_ip = argv[1] ;
	hname = reverse_resolve(argv[1]) ;
	if( hname == NULL )
		cerr << "Warning : Inverse host look up for " << argv[1] << " failed\n" ;
 }
 server.sin_family = AF_INET ;
 server.sin_port = htons( atoi(argv[2]) ) ;
 if( inet_aton(h_ip,&server.sin_addr) == 0 )
 {
	if( hname != NULL )
		cerr << hname << "(" << h_ip << ") is not a valid IPv4 address\n" , exit(1) ;
	else
		cerr << h_ip << " is not a valid IPv4 address\n" , exit(1) ;
 }
 bzero(&server.sin_zero,sizeof(server.sin_zero));
 if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
	perror("Socket creation failed") , exit(1) ;
 if( connect(sockfd,(sockaddr *)&server,sizeof(server)) < 0 )
 {
	if( hname != NULL )
		cerr << "Connection to " << hname << "(" << h_ip << ") on port 1235 failed : " << strerror(errno) << endl , exit(1) ;
	else
		cerr << "Connection to " << h_ip << " on port 1235 failed : " << strerror(errno) << endl  , exit(1) ;
 }
 string sync("ACCEPTED");
 int n ;
 char buf[9];
 cout << "Connected : waiting for the other side to accept.....\n" ;
 read(sockfd,buf,8) ;
 buf[8] = 0 ;
 if( string(buf) != sync )
 {
	if( hname != NULL )
		cerr << " Call not lifted by " << hname << "(" << h_ip << ")\n" ;
	else
		cerr << " Call not lifted by " << h_ip << "\n" ;
	close(sockfd) ;
	exit(1) ;
 }
 if( hname != NULL )
         cout << "Call accepted by " << hname << "(" << h_ip << ")\n" ;
 else
        cout << "Call accepted by " << h_ip << "\n" ;
 string line ;
 if( (fin = popen("cat file.wav 2>/dev/null","r")) == NULL )
 {
	perror("Error initializing audio recorder") ;
	cerr << "Call ended prematurely\n" ;
	close(sockfd) ;
	exit(1) ;
 }
 if( (fout = popen("aplay 2>/dev/null","w")) == NULL )
 {
	perror("Error initializing audio output device");
	cerr << "Call ended prematurely\n" ;
	close(sockfd) ;
	exit(1) ;
 }
 ind = fileno(fin) ;
 outd = fileno(fout) ;
 if( (cpid = fork()) == 0 )
 {
	signal(SIGTERM,SIG_IGN);
	signal(SIGINT,c_handle);
 	while( (n = read(sockfd,(char *)line.c_str(),1200)) > 0 )
    		write(outd,line.c_str(),n) ;
	exit(0);
 }
 else
 {
	signal(SIGINT,handle);
	signal(SIGCHLD,handle);
	signal(SIGPIPE,handle);
	signal(SIGTERM,handle);
	while( (n = read(ind,(char *)line.c_str(),1200)) > 0 )
		write(sockfd,line.c_str(),n) ;
 }
 return 0;
}
