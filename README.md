# privatechat

Questo progetto riguarda lo sviluppo di una chat privata tra utenti registrati al servizio.
Un utente gia` registrato puo` accede al servizio con le sue credenziali (username e password) ed ha la possibilita` di inviare messaggi ad altri utenti online.

Una volta effettuato l'accesso, l'utente puo scegliere tra diverse opzioni che il servizio mette a disposizione:
- users, mostra gli utenti online 
- chat, apre una chat con un utente che e` online
- exit, esce dalla chat 

Scegliendo "chat", l'utente avra la possibilita` di scegliere un utente a cui inviare messaggi, se l'utente risulta online lo scambio di messaggi avverra`.
In qualunque momento l'utente puo inviare "return" per tornare al menu principale.

- HOW

- Client 
Il client e` sviluppato in modo tale da permettere all'utente di inviare/ricevere messaggi contemporaneamente.
Una volta effettuato il setup della connessione al server, vengono creati due thread, ripettivamente "sender" e "receiver" che gestiscono, uno l'acquisizione dei messaggi da terminale e il loro invio e l'altro la ricezione e stampa a video degli stessi.
Una volta inviato il comando "exit" il client uscira` rilasciando la memoria allocata.

- Server 
il server si occupa di inoltrare i messaggi tra i vari client online.
Crea un thread per ogni connessione in ingresso su cui gestisce la stessa.
Una volta autenticato l'utente, invia al client un messaggio di benvenuto con le opzioni disponibili.
Quando arriva una richiesta di chat (open_chat), verifica che l'utente destinatario sia online, dopo di che parte la sessione di chat verso l'utente desiderato (start_comunication).
Il server puo` essere interrotto in qualunque momento inviando "shutdown" da terminale, in tal caso si spegnera` quando non ci saranno` piu` utenti online.


- HOW TO RUN

1. in utils.h definire SERVER_ADDRESS e SERVER_PORT

da terminale lanciare in ordine:
- make
- ./server
- ./client (almeno 2)

Effettuare il login (utenti registrati):

- leonardo
- francesco 
- nino
- matteo 
- maria
- paolo

Aprire una chat:
- inviare "chat" dal menu principale.
- inviare l'username dell'utente con cui si desidera chattare oppure "users" per vedere gli utenti online.

Chiudere una chat:
- inviare "return" da una chat aperta

Disconnettersi dal server:
- inviare "exit" dal menu principale


Spengere il server: 
- inviare "shutdown"



