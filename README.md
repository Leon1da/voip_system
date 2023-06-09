# VoIP system
This project was developed as part of the Operating System course taught by Professor Giorgio Grisetti at Sapienza University of Rome. The course details can be found at the following link: [Operating System Course - Sapienza University of Rome](https://sites.google.com/diag.uniroma1.it/sistemi-operativi-1819).

The VoIP system project aims to create a private communication system that includes both VoIP functionality and chat capabilities. Registered users can log in to the service using their username and password, enabling them to exchange messages with other online users.

Once logged in, users have access to the following options:
- "users": displays the list of online users.
- "chat": opens a chat session with an online user.
- "exit": allows the user to exit the chat.
- "video": a video chat service is planned for future implementation.

The project utilizes both TCP and UDP protocols to implement the chat and the VoIP communication, respectively.

### Compilation
To compile the project, define the SERVER_ADDRESS and SERVER_PORT variables in the 'utils.h' file, and then run the "make" command.

### Running
To run the system, start the server by executing the command "./server", and then launch two or more clients using the command "./client".

##### Registered Users
Here is a list of some registered users along with their corresponding username and password:
- leonardo leonardo
- francesco francesco
- nino nino
- matteo matteo
- maria maria
- paolo paolo

##### Opening a Chat
To initiate a chat, select the "chat" option from the main menu and provide the username of the user you wish to chat with. To see the list of online users, use the "users" command.

##### Closing a Chat
To close an open chat session, simply send the "return" command from within the chat.

##### Disconnecting a Client from the Server
To disconnect a client from the server, select the "exit" option from the main menu.

##### Server Shutdown
To shut down the server, send the "shutdown" command.

