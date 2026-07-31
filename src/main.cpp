#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

std::vector<unsigned char> header_parser(char buffer[512]) {
    std::vector<unsigned char> temp;
    for (int i = 0; i < 12; i++) {
        temp.push_back(buffer[i]);
    }
    return temp;
}

std::vector<unsigned char> question_answer_parser(char buffer[512], int bytesRead) {
    std::vector<unsigned char> temp;
    // question section
    for (int i = 12; i < bytesRead; i++) {
        temp.push_back(buffer[i]);
    }
    // answer section
    int upper = temp.size();
    for (int i = 0; i < upper; i++) {
        temp.push_back(temp[i]);
    }
    temp.insert(temp.end(), { 0, 0, 0, 60, 0, 4, 8, 8, 8, 8 });
    return temp;
}

int main() {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Disable output buffering
    setbuf(stdout, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!" << std::endl;

      // TODO: Uncomment the code below to pass the first stage
   int udpSocket;
   struct sockaddr_in clientAddress;

   udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (udpSocket == -1) {
       std::cerr << "Socket creation failed: " << strerror(errno) << "..." << std::endl;
       return 1;
   }

   // Since the tester restarts your program quite often, setting REUSE_PORT
   // ensures that we don't run into 'Address already in use' errors
   int reuse = 1;
   if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
       std::cerr << "SO_REUSEPORT failed: " << strerror(errno) << std::endl;
       return 1;
   }

   sockaddr_in serv_addr = { .sin_family = AF_INET,
                             .sin_port = htons(2053),
                             .sin_addr = { htonl(INADDR_ANY) },
                           };

   if (bind(udpSocket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0) {
       std::cerr << "Bind failed: " << strerror(errno) << std::endl;
       return 1;
   }

   int bytesRead;
   char buffer[512];
   socklen_t clientAddrLen = sizeof(clientAddress);

   while (true) {
       // s
       bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
       if (bytesRead == -1) {
           perror("Error receiving data");
           break;
       }

       buffer[bytesRead] = '\0';
       std::cout << "Received " << bytesRead << " bytes: " << buffer << std::endl;
       
       std::vector<unsigned char> response = header_parser(buffer);
       std::vector<unsigned char> question_and_answer = question_answer_parser(buffer, bytesRead);
       for (int i = 0; i < question_and_answer.size(); i++) {
           response.push_back(question_and_answer[i]);
       }
       // shifting QR while leaving all other fields untouched
       response[2] |= 128;

       // extracting the numerical value of OPCODE
       int op_code = (response[2] & 120) >> 3;
       if (op_code == 0) {
           
       } else {
           response[3] = 4;
       }
       response[7] = 1;
       if (sendto(udpSocket, response.data(), response.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
           perror("Failed to send response");
       }
   }

   close(udpSocket);

    return 0;
}
