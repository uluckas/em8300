#define FIFOTYPE_AUDIO 1
#define FIFOTYPE_VIDEO 2

struct video_fifoslot_s {
    unsigned flags;
    unsigned physaddress_hi;
    unsigned physaddress_lo;
    unsigned slotsize;
};

struct audio_fifoslot_s {
    unsigned physaddress_hi;
    unsigned physaddress_lo;
    unsigned slotsize;
};

struct pts_fifoslot_s {
    unsigned streamoffset_hi;
    unsigned streamoffset_lo;
    unsigned pts_hi;
    unsigned pts_lo;
};

struct fifo_s {
    int valid;

    int type;
    int nslots;
    union {
	struct video_fifoslot_s *v;
	struct audio_fifoslot_s *a;
	struct pts_fifoslot_s *pts;
    } slots;
    int slotptrsize;
    int slotsize;

    int start;
    int *writeptr;
    int *readptr;
    int localreadptr;
    int threshold;

    char *fifobuffer;

#if LINUX_VERSION_CODE < 0x020314    
    struct wait_queue *wait;
#else
    wait_queue_head_t wait;
#endif	
    int waiting;
};

struct em8300_s;

/*
  Prototypes
*/
int em8300_fifo_init(struct em8300_s *em, struct fifo_s *f,
		     int start, int wrptr, int rdptr,
		     int pcisize, int slotsize, int fifotype);

struct fifo_s * em8300_fifo_alloc(void);
void em8300_fifo_free(struct fifo_s *f);

int em8300_fifo_write(struct fifo_s *fifo, int n, const char *userbuffer,
		      int swapbyteorder, int flags);
int em8300_fifo_writeblocking(struct fifo_s *fifo, int n,
			      const char *userbuffer,int swapbyteorder, int flags);
int em8300_fifo_check(struct fifo_s *fifo);

int em8300_fifo_sync(struct fifo_s *fifo);

int em8300_fifo_freeslots(struct fifo_s *fifo);

void em8300_fifo_statusmsg(struct fifo_s *fifo, char *str);
