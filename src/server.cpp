#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <fstream>
#include <openssl/md5.h>
#include "rpc/server.h"
using namespace std;

class ThreadPool {
public:
  // todo: implement a ThreadPool or get it from a library
  //       for later work, multithreading is the last priority
};

class File{
public:
    // data members of a File type, adjust according to your needs
    string name, file_id, author, location_on_disc, last_update_time;
    size_t size;
    unsigned int num_downloads;
    vector<string> access_to;

    // constructor to the File Class
    File(const string name, const string file_id, const string author, const string location_on_disc, const string last_update_time, const size_t size, const unsigned int num_downloads) {
      this->access_to = access_to;
      this->author = author;
      this->file_id = file_id;
      this->last_update_time = last_update_time;
      this->location_on_disc = location_on_disc;
      this->name = name;
      this->num_downloads = num_downloads;
      this->size = size;
    }
};

// function to split a string based on a particular delimeter
// contributed by @Amit
vector<string> split(string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}
class Server{
private:
    vector<string> server_ips;
    unordered_set<string> file_ids;
    unordered_map<string, File> file_table;
    unordered_map<string, string> file_hashes; // maps file_hash->file_id
    // own ThreadPool of threads to submit tasks to
    ThreadPool thread_exec;
    unordered_map<string, string> user_ids;
    std::string __getFileContent(string filepath) {
      std::ifstream file(filepath);
      std::stringstream content;

      if (file.is_open()) {
          content << file.rdbuf();
          file.close();
      } else {
          throw std::runtime_error("Unable to open file: " + filepath);
      }

      return content.str();
    }
    // function to check user with user_id present or not
    bool __checkUserWithUserID(string user_id) {
      return user_ids.count(user_id);
    }
    string __getFileID(string content) {
      unsigned char digest[MD5_DIGEST_LENGTH];
      MD5((const unsigned char*)input.c_str(), input.length(), digest);

      std::stringstream ss;
      for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
          ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
      }
      string res_file_id = ss.str();
      if(res_file_id>20) {
        res_file_id = res_file_id.substr(0, 20);
      }
      return res_file_id;
    }
public:
    // constructor to setup things
    Server() {
        // update the user logins in the server using FS based database
        ifstream user_db("user_db.txt");
        string line;
        while(user_db >> line) {
          vector<string> splitted = split(line, " ");
          // name user_id passwd <- user_db.txt file
          user_ids[splitted[1]] = splitted[2];
        }
    }
    
    // function to signin with user_id, password
    // contributed by @Amit
    bool signInWithUserIDPassword(string user_id, string password) {
      if(user_ids.count(user_id) && user_ids[user_id] == password) return true;
      return false;
    }
    // function to register with name, user_id, password
    // contributed by @Ajay
    bool registerWithUserIDPassword(string name, string user_id, string password) {
      if (!__checkUserWithUserID(user_id)) {
        user_ids[user_id] = password;
        ofstream user_db("user_db.txt", ios::app);
        if (user_db.is_open()) {
          user_db << name << " " << user_id << " " << password << "\n";
          user_db.close();
          return true;
        }
      }
      return false;
    }
    // function to replicate data across other servers, takes no argument and no return
    // must register this as a periodic event (cron job)
    void replicateDataAcrossServer() {};
    // function to handleDownload
    string handleDownload(string file_id) {
      if (file_table.count(file_id)) {
        // send contents of file
        return __getFileContent(file_table[file_id].location_on_disc);
      } else {
        cout << "[log] File resquest for {" << file_id << "}: File not found!" << endl;
        return "404: NOT FOUND";
      }
    }
    // function to handleUpload
    void handleUpload(string name, string author, string permissions, unsigned int size, string content) {
      //hash the content and generate a fileId
      string file_id = __getFileID(content);
      string location_on_disc = "/uploaded_files/"+file_id+"-"+name;
      std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      string curr_time = std::ctime(&end_time);
      
      File new_file(name, file_id, author, string name, file_id, author, location_on_disc, curr_time, size, 0);
      file_ids[file_id] = new_file;
      ofstream write_file(location_on_disc);
      write_file << content;
    }
    // function to check if file with the hash is already present on server or not
    // in the file_hashes map
    bool checkFilePresent(string file_hash) {};
};

int main(int argc, char *argv[]) {
  // Creating a server that listens on port 8080
  rpc::server srv(8080);

  Server serv_instance;
  
  srv.bind("signin",
  [&serv_instance](string user_id, string password){
    return serv_instance.signInWithUserIDPassword(user_id, password);
  });

  srv.bind("register", 
  [&serv_instance](string name, string user_id, string password){
    return serv_instance.registerWithUserIDPassword(name, user_id, password);
  });
  
  srv.bind("upload", 
  [&serv_instance](string name, string author, string permissions, unsigned int size, string content){ 
    serv_instance.handleUpload(name, author, permissions, size, content); 
  });

  srv.bind("download",
  [&serv_instance](string file_id){ 
    return serv_instance.handleDownload(file_id); 
  });
  

  // Run the server loop.
  srv.run();

  return 0;
}
