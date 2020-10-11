# privatechat

Questo progetto riguarda lo sviluppo di una chat privata tra utenti registrati al servizio.
Un utente gia' registrato puo' accede al servizio con le sue credenziali (username e password) ed ha la possibilita` di inviare messaggi ad altri utenti online.

Una volta effettuato l'accesso, l'utente puo scegliere tra diverse opzioni che il servizio mette a disposizione:
- users, mostra gli utenti online 
- chat, apre una chat con un utente online
- call, effettua una chiamata ad un utente online (*)
- exit, esce dalla chat 

**(*) per quanto riguarda la comunicazione vocale, viene effettuata una connessione "P2P" tra host e host. Per il suo corretto funzionamento e' necessario che venga configurato un port forwarding sulla porta 30020 (piu` host della stessa nella stessa LAN non possono effettuare comunicazioni vocali verso l'esterno contemporaneamente, o per meglio dire non potranno essere contattati dall'esterno).
Se l'applicativo viene utilizzato in LAN non e' necessario il port forwarding.**

Digitando "chat", l'utente avra la possibilita' di digitare l'username dell'utente a cui vuole inviare il messaggio, e il contenuto del messaggio stesso.
A questo punto se l'utente risulta online il messaggio verra` inviato.

Digitando "users", l'utente richiedera' al server una lista con gli utenti attualmente connessi.

Digitando "call",  l'utente avra la possibilita' di digitare l'username dell'utente che vuole contattare.
Se l'utente e' online e accetta la chiamata avra' inizio la comunicazione.

Digitando "quit", l'utente abbandonera`il servizio. 

**HOW**
- Client 
Il client e' sviluppato in modo tale da permettere all'utente di inviare/ricevere messaggi contemporaneamente.
Una volta effettuato il setup della connessione al server, viene creato un thread (sender) che si occupa dell'acquisizione dei comandi inseriti dalla shell.
Il processo invece restera' in ascolto dei messaggi in arrivo dal server.
Una volta inviato il comando "exit" il client uscira` rilasciando la memoria allocata.

- Server 
Il server si occupa di:
	- autenticare i client che si connettono
	- indirizzare i messaggi che gli arrivano dai clients ai clients a cui sono destinati.

Autenticazione
Il server verifica le credenziali che gli sono arrivate dal client connesso e le confronta con quelle nel database, se le       credenziali sono corrette invia un messaggio di successo al client, altrimenti gli invia un messaggio di errore.

Routing
Il server riceve il messaggio legge mittente e destinatario, controlla che il destinatario sia online, in caso positivo gli spedisce il messaggio in caso negativo comunica al client mittente che c'e` un errore
	
Il server puo essere interrotto in qualsiasi momento inviando il segnale SIGINT.
Quando cio` accade invia un messaggio di uscita a tutti i client connessi.


**HOW TO RUN**
- cmake .
- make
- ./server
- ./client <server_address>     (*)

(*) per lanciare il client, il campo <server_address> e' facoltativo, l'indirizzo di default e` quello locale.
