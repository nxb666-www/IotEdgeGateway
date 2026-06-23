#include "mongoose.h"

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sub {
  struct sub *next;
  struct mg_connection *c;
  struct mg_str topic;
  uint8_t qos;
};

static struct sub *s_subs = NULL;
static int s_signo = 0;

static void signal_handler(int signo) {
  s_signo = signo;
}

static size_t next_topic(struct mg_mqtt_message *msg, struct mg_str *topic,
                         uint8_t *qos, size_t pos) {
  unsigned char *buf = (unsigned char *) msg->dgram.buf + pos;
  size_t new_pos;

  if (pos >= msg->dgram.len || pos + 2 > msg->dgram.len) return 0;

  topic->len = (size_t) (((unsigned) buf[0]) << 8 | buf[1]);
  topic->buf = (char *) buf + 2;
  new_pos = pos + 2 + topic->len + (qos == NULL ? 0 : 1);

  if (new_pos > msg->dgram.len) return 0;
  if (qos != NULL) *qos = buf[2 + topic->len];
  return new_pos;
}

static size_t next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                       uint8_t *qos, size_t pos) {
  uint8_t tmp;
  return next_topic(msg, topic, qos == NULL ? &tmp : qos, pos);
}

static void free_sub(struct sub *sub) {
  if (sub == NULL) return;
  free((void *) sub->topic.buf);
  free(sub);
}

static bool mqtt_topic_match(struct mg_str topic, struct mg_str filter) {
  size_t ti = 0, fi = 0;

  while (fi < filter.len) {
    if (filter.buf[fi] == '#') {
      return fi == filter.len - 1;
    }

    if (filter.buf[fi] == '+') {
      while (ti < topic.len && topic.buf[ti] != '/') ti++;
      fi++;
      if (fi < filter.len && filter.buf[fi] == '/') {
        if (ti >= topic.len || topic.buf[ti] != '/') return false;
        ti++;
        fi++;
      }
      continue;
    }

    if (ti >= topic.len || topic.buf[ti] != filter.buf[fi]) return false;
    ti++;
    fi++;
  }

  return ti == topic.len;
}

static void broker_fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_MQTT_CMD) {
    struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;

    switch (mm->cmd) {
      case MQTT_CMD_CONNECT: {
        uint8_t response[] = {0, 0};
        mg_mqtt_send_header(c, MQTT_CMD_CONNACK, 0, sizeof(response));
        mg_send(c, response, sizeof(response));
        MG_INFO(("CONNECT fd=%lu", (unsigned long) c->fd));
        break;
      }

      case MQTT_CMD_SUBSCRIBE: {
        size_t pos = 4;
        uint8_t qos = 0;
        uint8_t resp[256];
        struct mg_str topic;
        int num_topics = 0;

        while (num_topics < (int) sizeof(resp) &&
               (pos = next_sub(mm, &topic, &qos, pos)) > 0) {
          struct sub *sub = (struct sub *) calloc(1, sizeof(*sub));
          if (sub == NULL) break;

          sub->c = c;
          sub->topic = mg_strdup(topic);
          sub->qos = qos;
          LIST_ADD_HEAD(struct sub, &s_subs, sub);

          resp[num_topics++] = qos;
          MG_INFO(("SUB fd=%lu topic=%.*s", (unsigned long) c->fd,
                   (int) sub->topic.len, sub->topic.buf));
        }

        mg_mqtt_send_header(c, MQTT_CMD_SUBACK, 0, (uint32_t) num_topics + 2);
        uint16_t id = mg_htons(mm->id);
        mg_send(c, &id, 2);
        mg_send(c, resp, (size_t) num_topics);
        break;
      }

      case MQTT_CMD_PUBLISH: {
        MG_INFO(("PUB topic=%.*s payload=%.*s", (int) mm->topic.len,
                 mm->topic.buf, (int) mm->data.len, mm->data.buf));

        for (struct sub *sub = s_subs; sub != NULL; sub = sub->next) {
          if (mqtt_topic_match(mm->topic, sub->topic)) {
            struct mg_mqtt_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.topic = mm->topic;
            opts.message = mm->data;
            opts.qos = 0;
            opts.retain = false;
            mg_mqtt_pub(sub->c, &opts);
          }
        }
        break;
      }

      case MQTT_CMD_PINGREQ:
        mg_mqtt_send_header(c, MQTT_CMD_PINGRESP, 0, 0);
        break;

      case MQTT_CMD_DISCONNECT:
        c->is_draining = 1;
        break;
    }
  } else if (ev == MG_EV_CLOSE) {
    struct sub *sub = s_subs;
    while (sub != NULL) {
      struct sub *next = sub->next;
      if (sub->c == c) {
        LIST_DELETE(struct sub, &s_subs, sub);
        free_sub(sub);
      }
      sub = next;
    }
  }
}

int main(int argc, char **argv) {
  const char *port = argc > 1 ? argv[1] : "1884";
  char listen_url[64];
  struct mg_mgr mgr;

  snprintf(listen_url, sizeof(listen_url), "mqtt://0.0.0.0:%s", port);

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  mg_mgr_init(&mgr);
  MG_INFO(("IotEdgeGateway MQTT broker listening on %s", listen_url));

  if (mg_mqtt_listen(&mgr, listen_url, broker_fn, NULL) == NULL) {
    MG_ERROR(("listen failed: %s", listen_url));
    mg_mgr_free(&mgr);
    return 1;
  }

  while (s_signo == 0) {
    mg_mgr_poll(&mgr, 100);
  }

  mg_mgr_free(&mgr);
  return 0;
}
