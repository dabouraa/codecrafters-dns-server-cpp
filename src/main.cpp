#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

std::vector<unsigned char> header_parser(char buffer[512]) {
    std::vector<unsigned char> temp;
    for (int i = 0; i < 12; i++) {
        temp.push_back(buffer[i]);
    }
    return temp;
}

std::pair<std::vector<unsigned char>, int> read_label(char buffer[512], int finalpos) {
    std::vector<unsigned char> temp;
    
    for (int i = finalpos;; i++) {
        if (buffer[i] == 0) {
            temp.push_back(buffer[i]);
            finalpos = i;
            break;
        } else if (((buffer[i] & 192) >> 6) == 3) {
            int offset = ((buffer[i] & 63) << 8) | buffer[i+1];
            auto[temp2, finalpos2] = read_label(buffer, offset);
            temp.insert(temp.end(), temp2.begin(), temp2.end());
            finalpos = i + 1;
            break;
        } else {
            temp.push_back(buffer[i]);
        }
    }
    return {temp, finalpos};
}

// std::vector<unsigned char> handle_answer(std::vector<unsigned char> temp) {
//     std::vector <unsigned char> answers = temp;
//     answers.insert(answers.end(), { 0, 0, 0, 60, 0, 4, 8, 8, 8, 8 });
//     return answers;
// }
std::vector<unsigned char> question_answer_parser(char buffer[512], int bytesRead, int resolver_socket, sockaddr_in resolver_addr, char resolver_buffer[512], int resolver_bytes, socklen_t clientAddrLen) {
    std::vector<unsigned char> temp_answers;
    std::vector<std::vector<unsigned char>> questions;
    int id;
    int qdcount = buffer[5];
    int pos = 12;

    for (int q = 0; q < qdcount; q++) {
        std::vector<unsigned char> temp_query = { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
        temp_query[0] = buffer[0];
        temp_query[1] = buffer[1];
        auto [name, endpos] = read_label(buffer, pos);
        for (int i = endpos + 1; i < endpos + 5; i++) { name.push_back(buffer[i]); }
        temp_query.insert(temp_query.end(), name.begin(), name.end());
        sendto(resolver_socket, temp_query.data(), temp_query.size(), 0, reinterpret_cast<struct sockaddr*>(&resolver_addr), sizeof(resolver_addr));
        resolver_bytes = recvfrom(resolver_socket, resolver_buffer, 512, 0, reinterpret_cast<struct sockaddr*>(&resolver_addr), &clientAddrLen);
        if (resolver_bytes == -1) {
            perror("Erorr recieving data from resolver");
            continue;
        }
        auto [throwaway, idx] = read_label(resolver_buffer, 12);
        idx += 5;
        for (int i = idx; i < resolver_bytes; i++) {
            temp_answers.push_back(resolver_buffer[i]);
        }
        questions.push_back(name);
        pos = endpos + 5;
    }

    std::vector<unsigned char> res;
    for (auto &q : questions) { res.insert(res.end(), q.begin(), q.end()); }
    res.insert(res.end(), temp_answers.begin(), temp_answers.end());
    // for (auto &q : questions) { auto a = handle_answer(q); res.insert(res.end(), a.begin(), a.end()); }
    return res;
}

int main(int argc, char* argv[]) {
    std::string ip;
    int port_num;
    // Extracting resolver ip and port
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--resolver") {
            std::string addr = std::string(argv[i+1]);
            int pos = addr.find(":");
            
            ip = addr.substr(0, pos); // extracts all characters from index 0 to the colon in ip:port
            std::string port = addr.substr(pos + 1); // extracts all characters starting past the colon and running to the end

            port_num = std::stoi(port);
            
            
        }
    }

    sockaddr_in resolver_addr = {}; 
    resolver_addr.sin_family = AF_INET;
    resolver_addr.sin_port = htons(port_num);
    inet_pton(AF_INET, ip.c_str(), &resolver_addr.sin_addr);    

    int resolver_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
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
   char resolver_buffer[512];
   int resolver_bytes;

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
       std::vector<unsigned char> question_and_answer = question_answer_parser(buffer, bytesRead, resolver_socket, resolver_addr, resolver_buffer, resolver_bytes, clientAddrLen);
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
       response[5] = buffer[5];
       response[7] = buffer[5];
       if (sendto(udpSocket, response.data(), response.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
           perror("Failed to send response");
       }
   }

   close(udpSocket);

    return 0;
}
