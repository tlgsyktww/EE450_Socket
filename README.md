
# Socket Programming Project, Fall 2022
web registration system

In this project, a client makes a request to a central server, authenticates an account and then queries course information. The central server in turn interacts with three other backend servers for information extraction and data processing.

## What I have done

I have already finished requires phases in description:

1. **Phase 1:**  The client establishes a TCP connection with serverM, and serverM establishes UDP connections with the three backend servers (Server C, Server CS and Server EE) respectively. The client sends the username and password to the main server, and the main server encrypts the information and sends it to serverC.
2. **Phase 2A:** serverC sends the result of the authentication request to serverM over a UDP connection.
3. **Phase 2B:** serverM sends the result of the authentication request to the client over a TCP connection.
4. **Phase 3A:** If the verification exceeds 3 times, the client ends the process; if the user information is verified, the client fills in the course information to be queried according to the display and sends it to serverM.
5. **Phase 3B:** serverM sends the query information to the backend department server (serverCS or serverEE depending on the course department) through UDP connection.
6. **Phase 4A:** The backend department server sends the query result back to serverM through UDP connection.
7. **Phase 4B:** server M receives the result and sends the result to the client through TCP connection.
8. **Extal Credits:** The user can query information of multiple courses at one time, and it receives the information of each course inquired in order from the backend server of the corresponding department.

## Code Files and their functions

```
* client.c
    Code for client has following functions:
    1. Send username and password to serverM using TCP.
    2. Get user verification result from serverM.
    3. Send course information inquiry (single course or multiple courses) to serverM using TCP.
    4. Accept course inquiry results from serverM.

* serverM.c
    Code for serverM has following functions:
    1. Receive login requests from client using TCP.
    2. Encrypt user information.
    3. Communicate with backend serverC, use UDP to get user verification results.
    4. Send client the validation result using TCP.
    5. Receive course inquiry requests from client using TCP.
    6. Communicate with backend serverCS/EE, use UDP to get course search results.
    7. Send client the course search results using TCP.

* serverC.c
    Code for serverC has following functions:
    1. Read "cred.txt", get required message.
    2. Communicate with serverM by UDP, receive and send relevant information.

* serverCS.c
    1. Read "cs.txt", get required message.
    2. Communicate with serverM by UDP, receive and send relevant information. 

* serverEE.c
    Code for serverEE.c has almost the same as serverCS.c but the UDP port and file are different.  
```


## Format of message exchange

**client -> serverM** 

1. Authentication: `<username> <password>` 

   User account information is sent together, separated by spaces.

2. Single Course Query: `1 <course code> <category>`

   Single-course query request is send together. 1 means to query one course. Each parameter is separated by a space.

3. Multi-Course Query:

   `<number of courses>  <course code1> <course  code2>...` 

   For multiple courses in one query, the information is delivered at one time. The first parameter represents the total number of courses in this query. Each parameter is separated by a space.

**serverM -> serverC** 

1. Authentication: `<encrypted username> <encrypted password>` 

   Encrypted user account information is sent together, separated by spaces.

**serverM -> serverCS & EE**

1. Single Course Query: `<course code> <category>`

   serverM requests the category results of the corresponding course from the server of the department where the course is located, separated by spaces.

2. Multi-Course Query: `<course code> all` 

   For each course in the multi-course request, serverM sends a query request to its department, and the category is all.

**serverC -> serverM** 

1. No username: `FAIL_NO_USER` 
2. Password does not match: `FAIL_PASS_NO_MATCH` 
3. The authentication request has passed: `PASS` 

**serverCS & EE -> serverM** 

No course found: `FAIL_NO_COURSE` 

In other cases, the query result is sent directly.

**serverM -> client** 

The result sent to the client is consistent with what serverM received from the backend server.


## Idiosyncrasy in project

1. The `cred.txt` file read by serverC **MUST NOT** have a blank line at the end, and the file should end at the password of last user, otherwise it will lead to a deviation in reading the last line of information.

2. Since the backend server needs to read the file, the `cat` command *maybe* required before execution.

   For example: 

   `make all`

    `cat cred.txt` 

    `./serverC` 

3. To handle the variable strings, there are lot of buffer with suitable size (based on received input files). If a single message or file exceeds the set buffer, the program will crash.

4. In multiple course queries, the program has the ability to handle wrong course codes of normal length. But wrong code beyond 5 digit length can cause some problems.


## Resued Code

The implementation of TCP and UDP connection according to **Beej’s Guide to Network Programming**.  
