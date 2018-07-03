## This is a project I have done for a second year sourse work in university.

### It consists of three parts: 

- server - server application in C++. It binds a socket to all available interfaces and listens for incoming connections.
  When a connection was established it accepts a file from a client, saves it on a hard drive and closes a connection.
  
- client - client socket application in C++. It also binds a socket just as the server app, creates a connection with the
  server and sends files.
  
- DataBackup - client application in WinAPI. It provides the ability to input server ip:port to connect to and choose files 
  to send via the client socket app
  
  
In the future I am planning to refactor all of the code, because it is very difficult to read at the moment, add the possibility
to send directories rather than single files and also fix a bug where a progress of file transfer sometimes gets stuck.
  
  This project will be revived when I set up a personal server in my home, as it will give me motivation to keep working on it.
