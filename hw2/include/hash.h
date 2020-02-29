/* Defines for the agef hashing functions.

   SCCS ID	@(#)hash.h	1.6	7/9/87
 */

#define BUCKETS		257	/* buckets per hash table */
#define TABLES		50	/* hash tables */
#define EXTEND		100	/* how much space to add to a bucket */

struct hbucket {
    int             length;	/* key space allocated */
    int             filled;	/* key space used */
    ino_t          *keys;
};

struct htable {
    dev_t           device;	/* device this table is for */
    struct hbucket  buckets[BUCKETS];	/* the buckets of the table */
};

#define OLD	0		/* inode was in hash already */
#define NEW	1		/* inode has been added to hash */

//ANSI C PROTOTYPES HERE
int h_enter(dev_t, ino_t);
void h_stats();
//ANSI C PROTOTYPES HERE
