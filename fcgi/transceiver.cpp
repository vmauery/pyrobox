//! \file transceiver.cpp Defines member functions for Fastcgipp::Transceiver
/***************************************************************************
* Copyright (C) 2007 Eddie                                                 *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/


#include <transceiver.hpp>
#include <util.hpp>
#include <boost/bind.hpp>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <iostream>

static int inotify_fd;

extern int prog_argc;
extern char **prog_argv;
extern char **prog_env;

bool Fastcgipp::Transceiver::monitor_binary() {
	static char *basename = NULL;
	static char exepath[256];
	static char **argv = NULL;

	int bytes_read;
	int i;
	char buffer[4096];


	if (!prog_argc || !prog_env) return false;
	if (!basename) {
		memset(exepath, 0, sizeof(exepath));
		bytes_read = readlink("/proc/self/exe", exepath, sizeof(exepath));
		basename = strstr(exepath, " (deleted)");
		if (basename) {
			*basename = 0;
		}
		basename = rindex(exepath, '/') + 1;
		std::cerr << "basename: " << basename << std::endl;
	}

	bytes_read = read(inotify_fd, &buffer, sizeof(buffer));
	if (bytes_read > 0) {
		i = 0;
		while (i < bytes_read) {
			struct inotify_event *e = (struct inotify_event *) &buffer[i];
			if (strcmp(e->name, basename) == 0) {
				struct stat sb;
				if (stat(exepath, &sb) < 0) {
					perror("stat");
				} else {
					if (sb.st_mode & S_IXUSR) {
						struct tm t;
						time_t now = time(NULL);
						localtime_r(&now, &t);
						std::cerr << t.tm_mday << "-" << t.tm_mon << "-" << t.tm_year
								  << " " << t.tm_hour << ":" << t.tm_min << ":" << t.tm_sec
								  << ": found newer file, reloading..." << std::endl;
						stop();
						for (i = 0; i < 10; i++) {
							execve(exepath, prog_argv, prog_env);
							std::cerr << "errno = " << errno << std::endl;
							switch (errno) {
								case ETXTBSY:
									usleep(500000);
									break;	

								default:
									perror("execve");
									continue;
							}
						}
						exit(1);
					}
				}
			}
			i += sizeof(*e) + e->len;
		}
	}
	return true;
}

void Fastcgipp::Transceiver::stop() {
	std::cerr << "stopping connections" << std::endl;
	struct sockaddr_un saddr;
	socklen_t len = sizeof(saddr);
	if (socket) {
		shutdown(socket, SHUT_RDWR);
		fcntl(socket, F_SETFD, FD_CLOEXEC);
		if (!getsockname(socket, (struct sockaddr *)&saddr, &len)) {
			if (saddr.sun_family == AF_UNIX) {
				close(socket);
				unlink(saddr.sun_path);
			}
		}
		close(socket);
	}
	close(wake_up_fd_out);
	close(wake_up_fd_in);
	close(inotify_fd);
	log_close();
}

void Fastcgipp::Transceiver::terminate() {
	std::cerr << "quitting" << std::endl;
	stop();
	exit(0);
}

int Fastcgipp::Transceiver::transmit()
{
	while(1)
	{{
		Buffer::Send_block send_block(buffer.request_read());
		if(send_block.size)
		{
			ssize_t sent = write(send_block.fd, send_block.data, send_block.size);
			if(sent<0)
			{
				if(errno==EPIPE)
				{
					poll_fds.erase(std::find_if(poll_fds.begin(), poll_fds.end(), equals_fd(send_block.fd)));
					fd_buffers.erase(send_block.fd);
					sent=send_block.size;
				}
				else if(errno!=EAGAIN) throw Exceptions::Socket_write(send_block.fd, errno);
			}

			buffer.free_read(sent);
			if(sent!=send_block.size)
				break;
		}
		else
			break;
	}}

	return buffer.empty();
}

void Fastcgipp::Transceiver::Buffer::secure_write(size_t size, Protocol::Full_id id, bool kill)
{
	write_it->end+=size;
	if(min_block_size>(write_it->data.get()+Chunk::size-write_it->end) && ++write_it==chunks.end())
	{
		chunks.push_back(Chunk());
		--write_it;
	}
	frames.push(Frame(size, kill, id));
}

bool Fastcgipp::Transceiver::handler()
{
	using namespace std;
	using namespace Protocol;

	bool transmit_empty=transmit();

	int ret_val=poll(&poll_fds.front(), poll_fds.size(), 0);
	if(ret_val==0)
	{
		//monitor_binary();
		if(transmit_empty) return true;
		else return false;
	}
	if(ret_val<0) throw Exceptions::Poll(errno);
	
	std::vector<pollfd>::iterator poll_fd = find_if(poll_fds.begin(), poll_fds.end(), revents_zero);

	if(poll_fd->revents&POLLHUP)
	{
		fd_buffers.erase(poll_fd->fd);
		poll_fds.erase(poll_fd);
		return false;
	}
	
	int fd=poll_fd->fd;
	if(fd==socket)
	{
		sockaddr_un addr;
		socklen_t addrlen=sizeof(sockaddr_un);
		fd=accept(fd, (sockaddr*)&addr, &addrlen);
		fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL)|O_NONBLOCK)^O_NONBLOCK);
		
		poll_fds.push_back(pollfd());
		poll_fds.back().fd = fd;
		poll_fds.back().events = POLLIN|POLLHUP;

		Message& message_buffer=fd_buffers[fd].message_buffer;
		message_buffer.size=0;
		message_buffer.type=0;
	}
	else if(fd==wake_up_fd_in)
	{
		char x;
		read(wake_up_fd_in, &x, 1);
		return false;
	}
	else if(fd==inotify_fd) {
		if (!monitor_binary()) {
			poll_fds.erase(std::find_if(poll_fds.begin(), poll_fds.end(), equals_fd(inotify_fd)));
		}
		return false;
	}
	
	Message& message_buffer=fd_buffers[fd].message_buffer;
	Header& header_buffer=fd_buffers[fd].header_buffer;

	ssize_t actual;
	// Are we in the process of recieving some part of a frame?
	if(!message_buffer.data)
	{
		// Are we recieving a partial header or new?
		actual=read(fd, (char*)&header_buffer+message_buffer.size, sizeof(Header)-message_buffer.size);
		if(actual<0 && errno!=EAGAIN) throw Exceptions::Socket_read(fd, errno);
		if(actual>0) message_buffer.size+=actual;
		if(message_buffer.size!=sizeof(Header))
		{
			if(transmit_empty) return true;
			else return false;
		}

		message_buffer.data.reset(new char[sizeof(Header)+header_buffer.get_content_length()+header_buffer.get_padding_length()]);
		memcpy(static_cast<void*>(message_buffer.data.get()), static_cast<const void*>(&header_buffer), sizeof(Header));
	}

	const Header& header=*(const Header*)message_buffer.data.get();
	size_t needed=header.get_content_length()+header.get_padding_length()+sizeof(Header)-message_buffer.size;
	actual=read(fd, message_buffer.data.get()+message_buffer.size, needed);
	if(actual<0 && errno!=EAGAIN) throw Exceptions::Socket_read(fd, errno);
	if(actual>0) message_buffer.size+=actual;

	// Did we recieve a full frame?
	if(actual==needed)
	{		
		send_message(Full_id(header_buffer.get_request_id(), fd), message_buffer);
		message_buffer.size=0;
		message_buffer.data.reset();
		return false;
	}
	if(transmit_empty) return true;
	else return false;
}

void Fastcgipp::Transceiver::Buffer::free_read(size_t size)
{
	p_read+=size;
	if(p_read>=chunks.begin()->end)
	{
		if(write_it==chunks.begin())
		{
			p_read=write_it->data.get();
			write_it->end=p_read;
		}
		else
		{
			if(write_it==--chunks.end())
			{
				chunks.begin()->end=chunks.begin()->data.get();
				chunks.splice(chunks.end(), chunks, chunks.begin());
			}
			else
				chunks.pop_front();
			p_read=chunks.begin()->data.get();
		}
	}
	if((frames.front().size-=size)==0)
	{
		if(frames.front().close_fd)
		{
			poll_fds.erase(std::find_if(poll_fds.begin(), poll_fds.end(), equals_fd(frames.front().id.fd)));
			close(frames.front().id.fd);
			fd_buffers.erase(frames.front().id.fd);
		}
		frames.pop();
	}

}

void Fastcgipp::Transceiver::wake()
{
	char x;
	write(wake_up_fd_out, &x, 1);
}

Fastcgipp::Transceiver::Transceiver(int fd_, boost::function<void(Protocol::Full_id, Message)> send_message_)
:send_message(send_message_), poll_fds(3), socket(fd_), buffer(poll_fds, fd_buffers)
{
	socket=fd_;
	
	// Let's setup a in/out socket for waking up poll()
	int soc_pair[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, soc_pair);
	wake_up_fd_in=soc_pair[0];
	fcntl(wake_up_fd_in, F_SETFL, (fcntl(wake_up_fd_in, F_GETFL)|O_NONBLOCK)^O_NONBLOCK);	
	wake_up_fd_out=soc_pair[1];	

	std::cerr << "socket: " << socket << ", wake_in: " << wake_up_fd_in << ", wake_out: " << wake_up_fd_out << std::endl;
	
	fcntl(socket, F_SETFL, (fcntl(socket, F_GETFL)|O_NONBLOCK)^O_NONBLOCK);
	poll_fds[0].events = POLLIN|POLLHUP;
	poll_fds[0].fd = socket;
	poll_fds[1].events = POLLIN|POLLHUP;
	poll_fds[1].fd = wake_up_fd_in;

	// set up inotify to restart ourselves in the case of a newer version
	// XXX DEBUG
	inotify_fd = inotify_init();
	std::cerr << "inotify_fd = " << inotify_fd << std::endl;
	char mypath[256];
	memset(mypath, 0, sizeof(mypath));
	if (readlink("/proc/self/exe", mypath, sizeof(mypath)) < 0) {
		perror("readlink");
		close(inotify_fd);
		inotify_fd = -1;
	} else {
		*rindex(mypath, '/') = 0;
		std::cerr << "mypath = " << mypath << std::endl;
		if (inotify_add_watch(inotify_fd, mypath, IN_MOVED_TO | IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE) < 0) {
			perror("inotify_add_watch");
			close(inotify_fd);
			inotify_fd = -1;
		} else {
			//fcntl(inotify_fd, F_SETFL, fcntl(inotify_fd, F_GETFL) & ~O_NONBLOCK);	
			poll_fds[2].fd = inotify_fd;
			poll_fds[2].events = POLLIN|POLLHUP;
		}
	}
}
