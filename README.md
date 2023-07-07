# Web Server using C

This is a simple web server implementation in C that serves web pages to clients. The server is capable of handling multiple file types, error handling, and includes optimization techniques. It provides the same page for all client requests and serves different correct files based on the client's request.

## Prerequisites

- Web server is running and gives the same page for all client requests.
- Web server provides different and correct files for different client requests.
- Handling multiple file types.
- Error handling.
- Optimization.

## How to Run the Server

To run the server, please follow these steps:

1. Open a terminal or command prompt and navigate to the directory where you saved/cloned the project.

2. Compile the code by running the following command using a C compiler:
  
  ```
  gcc -o server server.c
  ```

3. Run the executable file by executing the following command in the terminal:
  
  ```
  ./server
  ```

4. Open a web browser and go to [http://localhost:8080/index.html](http://localhost:8080/index.html) to access the server.

5. To stop the server, you can press `Ctrl+C` in the terminal.

## Repository

Feel free to explore and modify the code according to your needs. If you encounter any issues or have suggestions, please create an issue on the repository.
