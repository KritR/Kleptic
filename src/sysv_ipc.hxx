#ifndef KLEPTIC_SYSV_IPC_HXX_
#define KLEPTIC_SYSV_IPC_HXX_
#include <string>
#include <tuple>
#include <utility>

#define IPC_MSG_SIZE 1024

namespace Kleptic::IPC {

struct message_text {
  int qid;
  char buf[IPC_MSG_SIZE];
};

struct msgbuf {
  int64_t message_type;
  struct message_text txt;
};

int get_queue(std::string key_file, char id);

int create_client();

void close_queue(int q);

std::tuple<int, int64_t, std::string> recv_ipc(int qid);

void send_ipc(int send_qid, int rec_qid, int64_t msg_type, std::string msg_data);
}  // namespace Kleptic::IPC
#endif  // KLEPTIC_SYSV_IPC_HXX_
