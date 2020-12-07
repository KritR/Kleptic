#include "sysv_ipc.hxx"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <string>
#include <tuple>
#include <utility>

namespace Kleptic::IPC {

int get_queue(std::string key_file, char id) {
  key_t msg_queue_key;
  // struct message message;

  if ((msg_queue_key = ftok(key_file.c_str(), id)) == -1) {
    perror("ftok");
    exit(1);
  }

  int qid;
  if ((qid = msgget(msg_queue_key, IPC_CREAT | 0660)) == -1) {
    perror("msgget");
    exit(1);
  }
  return qid;
}

std::tuple<int, int64_t, std::string> recv_ipc(int qid) {
  struct msgbuf msg;

  if (msgrcv(qid, &msg, sizeof(struct msgbuf), 0, 0) == -1) {
    perror("loggeripc msgrcv");
    // exit(1);
  }

  std::string msg_txt(msg.txt.buf);

  return std::make_tuple(msg.txt.qid, msg.message_type, msg_txt);
}

int create_client() {
  int client_qid;
  if ((client_qid = msgget(IPC_PRIVATE, 0660)) == -1) {
    perror("msgget: client_channel");
    exit(1);
  }
  return client_qid;
}

void close_queue(int q) {
  if (msgctl(q, IPC_RMID, NULL) == -1) {
    perror("loggeripc close");
  }
}

void send_ipc(int send_qid, int rec_qid, int64_t msg_type, std::string msg_data) {
  if (msg_data.size() >= (IPC_MSG_SIZE - 1)) {
    perror("message too large to send");
  }
  struct msgbuf msg;
  msg.message_type = msg_type;
  msg.txt.qid = send_qid;
  msg_data.copy(msg.txt.buf, msg_data.size());
  msg.txt.buf[msg_data.size()] = '\0';
  if (msgsnd(rec_qid, &msg, sizeof(struct msgbuf), 0) == -1) {
    perror("loggeripc msgsnd");
    // exit(1);
  }
}

}  // namespace Kleptic::IPC
