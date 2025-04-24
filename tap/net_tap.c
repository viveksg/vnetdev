#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <pthread.h>

#define BUFFER_SIZE 2048
#define INTERFACE_NAME "tap0"
int tap_fd;

char tap_tx_buffer[BUFFER_SIZE];
char tap_rx_buffer[BUFFER_SIZE];
pthread_mutex_t tap_fd_lock;
pthread_mutex_t nic_dev_lock;
pthread_mutex_t tx_buffer_lock;
pthread_mutex_t rx_buffer_lock;
pthread_t tx_thread;
pthread_t rx_thread;

void *tx_task(void *arg)
{

    while (1)
    {
        pthread_mutex_lock(&tap_fd_lock);
        pthread_mutex_lock(&nic_dev_lock);
        pthread_mutex_lock(&tx_buffer_lock);
        int nread = read(tap_fd, tap_tx_buffer, BUFFER_SIZE);

        if (nread > 0)
        {
            printf("Received %d bytes\n", nread);
            printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   tap_tx_buffer[0], tap_tx_buffer[1], tap_tx_buffer[2], tap_tx_buffer[3], tap_tx_buffer[4], tap_tx_buffer[5]);
            printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   tap_tx_buffer[6], tap_tx_buffer[7], tap_tx_buffer[8], tap_tx_buffer[9], tap_tx_buffer[10], tap_tx_buffer[11]);
            printf("EtherType: 0x%02x%02x\n", tap_tx_buffer[12], tap_tx_buffer[13]);
            // for(int i  = 0; i < nread;i++)
            // printf("%c", tap_tx_buffer[i]);
            printf("-----------------xxxxxxx--------------------------");
        }
        pthread_mutex_unlock(&tap_fd_lock);
        pthread_mutex_unlock(&nic_dev_lock);
        pthread_mutex_unlock(&tx_buffer_lock);
    }
}

int init_locks()
{
    if (pthread_mutex_init(&tap_fd_lock, NULL) != 0)
    {
        perror("cannot initialize tap lock");
        return -1;
    }
    if (pthread_mutex_init(&nic_dev_lock, NULL) != 0)
    {
        perror("cannot initialize nic_dev lock");
        return -1;
    }
    if (pthread_mutex_init(&tx_buffer_lock, NULL) != 0)
    {
        perror("cannot initialize tx_buffer lock");
        return -1;
    }
    return 0;
}

int main(void)
{

    if (init_locks())
    {
        perror("cannot initialize locks...\n");
        return -1;
    }
    if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        perror("Cannot obtain tap file descriptor\n");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    memcpy(&ifr.ifr_ifrn, INTERFACE_NAME, sizeof(INTERFACE_NAME));

    if (ioctl(tap_fd, TUNSETIFF, (void *)&ifr) < 0)
    {
        perror("ioctl error");
        close(tap_fd);
        return 0;
    }
    pthread_create(&tx_thread, NULL, tx_task, NULL);
    pthread_join(tx_thread, NULL);
    return 0;
}
