/******************************************************************************
**
** FILE NAME    : vectoring.c
** PROJECT      : UEIP
** MODULES      : MII0/1 + PTM Acceleration Package (VR9 PPA E5)
**
** DATE         : 09 OCT 2012
** AUTHOR       : Xu Liang
** DESCRIPTION  : Vectoring support for VDSL WAN.
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 09 OCT 2012  Xu Liang        Initiate Version
*******************************************************************************/



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/etherdevice.h>
#include <linux/pkt_sched.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*
 *  Chip Specific Head File
 */
#include "ifxmips_vectoring_stub.h"


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define DMA_PACKET_SIZE                         1600

#define ERB_MAX_BACHCHANNEL_DATA_SIZE           (1024 - 5)
#define ERB_BACHCHANNEL_DATA_OFF_LLC            (8 + 5)
#define ERB_MAX_PAYLOAD                         3600
#define ERB_SIZE                                ((int)&((struct erb_head *)0)->backchannel_data)

#define err(format, arg...)                     do { if ( (g_dbg_enable & DBG_ENABLE_MASK_ERR) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#define dbg(format, arg...)                     do { if ( (g_dbg_enable & DBG_ENABLE_MASK_DEBUG_PRINT) ) printk(KERN_WARNING __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#define ASSERT(cond, format, arg...)            do { if ( (g_dbg_enable & DBG_ENABLE_MASK_ASSERT) && !(cond) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )

#define DBG_ENABLE_MASK_ERR                     (1 << 0)
#define DBG_ENABLE_MASK_DEBUG_PRINT             (1 << 1)
#define DBG_ENABLE_MASK_ASSERT                  (1 << 2)
#define DBG_ENABLE_MASK_DUMP_SKB_TX             (1 << 9)
#define DBG_ENABLE_MASK_ALL                     (DBG_ENABLE_MASK_ERR | DBG_ENABLE_MASK_DEBUG_PRINT | DBG_ENABLE_MASK_ASSERT | DBG_ENABLE_MASK_DUMP_SKB_TX)

#define VECTOR_QUEUE_XMIT

#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
static unsigned int vector_timer_interval=32; /*in millseconds */
static unsigned int en_vector_tx_api=1;
static unsigned int en_vector_tx_api_test=0;
static unsigned int avm_mei_dsm_cb_func_call_cnt = 0;
static unsigned int avm_mei_dsm_cb_func_call_too_fast = 0;
static unsigned int avm_in_irq = 0;
static unsigned int avm_in_interrupt = 0;
static unsigned int avm_irqs_disabled = 0;
static unsigned int avm_irqs_queue_cnt = 0;
static unsigned int avm_tasklet_dequeue_cnt = 0;

static uint64_t avm_sum_delta_ms = 0;
static uint32_t avm_sum_delta_ms_count = 0;
#endif /* CONFIG_IFX_VECTOR_TIMER_CHECK*/



/*
 * ####################################
 *             Declaration
 * ####################################
 */

static int32_t mei_dsm_cb_func(uint32_t *p_error_vector);

static void register_netdev_event_handler(void);
static void unregister_netdev_event_handler(void);
static int netdev_event_handler(struct notifier_block *nb, unsigned long event, void *netdev);

static int proc_read_dbg(struct seq_file *, void *);
static int proc_write_dbg(struct file *file, const char __user *buf, size_t count, loff_t *data);
static int proc_read_dbg_seq_open(struct inode *, struct file *);
static void dump_data(void *data, unsigned int len, char *title);
static void dump_skb(struct sk_buff *skb, u32 len, char *title, int port, int ch, int is_tx, int enforce);


/*
 * ####################################
 *            Data Structure
 * ####################################
 */

struct erb_head {
    unsigned char   vce_mac[6];
    unsigned char   vtu_r_mac[6];
    unsigned short  length;
    unsigned char   llc_header[3];
    unsigned char   itu_t_oui[3];
    unsigned short  protocol_id;
    unsigned short  line_id;
    unsigned short  sync_symbol_count;
    unsigned char   segment_code;
    unsigned char   backchannel_data[0];    //  place holder of backchannel data
};



/*
 * ####################################
 *           Global Variable
 * ####################################
 */

static struct notifier_block g_netdev_event_handler_nb = {0};
static struct net_device *g_ptm_net_dev = NULL;

static int g_dbg_enable = 0;



/*
 * ####################################
 *            Local Function
 * ####################################
 */

/* ------------------------------------------------------------------------ */

#if defined(VECTOR_QUEUE_XMIT)
#define MAX_TASKLET_PACKETS 32

static struct sk_buff_head 	 vector_irq_queue;
static struct tasklet_struct vector_irq_tasklet;

static void vector_irq_tasklet_fn(unsigned long data __attribute__((unused)))
{
   int count = MAX_TASKLET_PACKETS;
   struct sk_buff *skb;

   while (count-- > 0 && (skb = skb_dequeue(&vector_irq_queue)) != 0) {
	   dev_queue_xmit(skb);
#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
	   avm_tasklet_dequeue_cnt ++;
#endif
   }
   if (skb_queue_len(&vector_irq_queue))
      tasklet_schedule(&vector_irq_tasklet);
}
#endif
/* ------------------------------------------------------------------------ */

static int mei_dsm_cb_func(unsigned int *p_error_vector)
{
    int rc = 0;
    struct sk_buff *skb_list = NULL;
    struct sk_buff *skb;
    struct erb_head *erb;
    unsigned int total_size, sent_size, block_size;
    unsigned int num_blocks;
    unsigned int segment_code;
    unsigned int i;



#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
    static int f_first_call=1;
    static unsigned long last_jiffies=0;



    avm_mei_dsm_cb_func_call_cnt++;

    if( f_first_call )
    {
        last_jiffies = jiffies;
        f_first_call = 0;
    }
    else
    {
        unsigned long curr_jiffies = jiffies;
        if( last_jiffies < curr_jiffies )
        {
            unsigned int delta_ms = jiffies_to_msecs(curr_jiffies - last_jiffies);
            avm_sum_delta_ms += delta_ms;
            avm_sum_delta_ms_count ++;
            if( delta_ms < vector_timer_interval ) /*Linux jiffies will overflow after 5 minutes*/
            {
                printk(KERN_ERR "Warning for vector timer %u ms less than expected %u ms. Current/last jiffies=%lu/%lu\n", delta_ms, vector_timer_interval, curr_jiffies, last_jiffies);
                avm_mei_dsm_cb_func_call_too_fast++;
            }

        }
        //update last_jiffies
        last_jiffies = curr_jiffies;
    }
    if( !en_vector_tx_api || en_vector_tx_api_test )  //tx API is tempariary disabled or just in test mode
    {
        dbg("en_vector_tx_api is %s\n", en_vector_tx_api?"Enabled":"Disabled" );
        return -EINVAL;
    }
#endif    /*CONFIG_IFX_VECTOR_TIMER_CHECK */
    if ( (g_dbg_enable & DBG_ENABLE_MASK_DEBUG_PRINT) )
    {
        dump_data(p_error_vector, 128, "vectoring raw data");
    }

    if ( g_ptm_net_dev == NULL )
    {
        err("g_ptm_net_dev == NULL");
        return -ENODEV;
    }

    erb = (struct erb_head *)(p_error_vector + 1);
    total_size = erb->length;
    if ( total_size < ERB_BACHCHANNEL_DATA_OFF_LLC || total_size > ERB_MAX_PAYLOAD )
    {
        err("p_error_vector[0] = %u, erb->length = %u", p_error_vector[0], total_size);
        return -EINVAL;
    }

    total_size -= ERB_BACHCHANNEL_DATA_OFF_LLC;
    num_blocks = (total_size + ERB_MAX_BACHCHANNEL_DATA_SIZE - 1) / ERB_MAX_BACHCHANNEL_DATA_SIZE;

    for ( i = 0; i < num_blocks; i++ ) {
        skb = dev_alloc_skb(DMA_PACKET_SIZE);
        if ( !skb ) {
            while ( (skb = skb_list) != NULL ) {
                skb_list = skb_list->next;
                skb->next = NULL;
                dev_kfree_skb_any(skb);
            }
            err("dev_alloc_skb fail");
            return -ENOMEM;
        }
        else {
            skb->next = skb_list;
            skb_list = skb;
        }
    }

    sent_size = 0;
    segment_code = 0;
    while ( (skb = skb_list) != NULL ) {
#if !defined(VECTOR_QUEUE_XMIT)
        int ret = 0;
#endif
        skb_list = skb_list->next;
        skb->next = NULL;

        block_size = min(total_size, sent_size + ERB_MAX_BACHCHANNEL_DATA_SIZE) - sent_size;
        skb_put(skb, block_size + ERB_SIZE);
        memcpy(skb->data, erb, ERB_SIZE);
        memcpy(((struct erb_head *)skb->data)->backchannel_data, &erb->backchannel_data[sent_size], block_size);
        sent_size += block_size;

        ((struct erb_head *)skb->data)->length = (unsigned short)(block_size + ERB_BACHCHANNEL_DATA_OFF_LLC);
        if ( skb_list == NULL )
            segment_code |= 0xC0;
        ((struct erb_head *)skb->data)->segment_code = segment_code;

        skb_reset_mac_header(skb);
        skb_set_network_header(skb, offsetof(struct erb_head, llc_header));
        skb->protocol = htons(ETH_P_802_2);

#if defined(VECTOR_QUEUE_XMIT)
#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
        if(irqs_disabled()){
            avm_irqs_disabled++;
        }

        if(in_irq()){
            avm_in_irq++;
        }

        if(in_interrupt()){
            avm_in_interrupt++;
        }
#endif /*--- defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK ---*/

        skb->dev = g_ptm_net_dev;
        skb->priority = TC_PRIO_CONTROL;
        dump_skb(skb, ~0, "vectoring TX", 0, 0, 1, 0);
        skb_queue_tail(&vector_irq_queue, skb);
        tasklet_schedule(&vector_irq_tasklet);
#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
        avm_irqs_queue_cnt++;;
#endif

#else /*--- #if defined(VECTOR_QUEUE_XMIT) ---*/

        skb->cb[13] = 0x5A; /* magic number indicating forcing QId */
        skb->cb[15] = 0x00; /* highest priority queue */
        skb->dev = g_ptm_net_dev;
        dump_skb(skb, ~0, "vectoring TX", 0, 0, 1, 0);
        ret = g_ptm_net_dev->netdev_ops->ndo_start_xmit(skb, g_ptm_net_dev);
        if (rc == 0)
            rc = ret;
#endif  /*--- #if defined(VECTOR_QUEUE_XMIT) ---*/

        segment_code++;
    }

    *p_error_vector = 0;    /* notify DSL firmware that ERB is sent */

    ASSERT(rc == 0, "dev_queue_xmit fail - %d", rc);
    return rc;
}

static void register_netdev_event_handler(void)
{
    g_netdev_event_handler_nb.notifier_call = netdev_event_handler;
    register_netdevice_notifier(&g_netdev_event_handler_nb);
}

static void unregister_netdev_event_handler(void)
{
    unregister_netdevice_notifier(&g_netdev_event_handler_nb);
}

static int netdev_event_handler(struct notifier_block *nb __attribute__((unused)), unsigned long event, void *netdev)
{
    struct net_device *netif;

    if ( event != NETDEV_REGISTER
        && event != NETDEV_UNREGISTER )
        return NOTIFY_DONE;

    netif = netdev_notifier_info_to_dev(netdev);
    if ( strcmp(netif->name, "dsl0") != 0 )
        return NOTIFY_DONE;

    g_ptm_net_dev = event == NETDEV_REGISTER ? netif : NULL;
    return NOTIFY_OK;
}

#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
static void ignore_space(char **p, int *len)
{
    while ( *len > 0 && (**p <= ' ' || **p == ':' || **p == '.' || **p == ',') )
    {
        (*p)++;
        (*len)--;
    }
}

static unsigned int get_number(char **p, int *len, int is_hex)
{
    unsigned int ret = 0;
    int n = 0;

    if ( (*p)[0] == '0' && (*p)[1] == 'x' )
    {
        is_hex = 1;
        (*p) += 2;
        (*len) -= 2;
    }

    if ( is_hex )
    {
        while ( *len && ((**p >= '0' && **p <= '9') || (**p >= 'a' && **p <= 'f') || (**p >= 'A' && **p <= 'F')) )
        {
            if ( **p >= '0' && **p <= '9' )
                n = **p - '0';
            else if ( **p >= 'a' && **p <= 'f' )
               n = **p - 'a' + 10;
            else if ( **p >= 'A' && **p <= 'F' )
                n = **p - 'A' + 10;
            ret = (ret << 4) | n;
            (*p)++;
            (*len)--;
        }
    }
    else
    {
        while ( *len && **p >= '0' && **p <= '9' )
        {
            n = **p - '0';
            ret = ret * 10 + n;
            (*p)++;
            (*len)--;
        }
    }

    return ret;
}
#endif /*--- #if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK ---*/

static int proc_read_dbg(struct seq_file *m, void *data __maybe_unused)
{

#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
    uint64_t avm_mean_time = avm_sum_delta_ms;
    if ( avm_sum_delta_ms_count )
        do_div(avm_mean_time, avm_sum_delta_ms_count);
#endif /*CONFIG_IFX_VECTOR_TIMER_CHECK */

    seq_printf(m , "error print      - %s\n", (g_dbg_enable & DBG_ENABLE_MASK_ERR)              ? "enabled" : "disabled");
    seq_printf(m , "debug print      - %s\n", (g_dbg_enable & DBG_ENABLE_MASK_DEBUG_PRINT)      ? "enabled" : "disabled");
    seq_printf(m , "assert           - %s\n", (g_dbg_enable & DBG_ENABLE_MASK_ASSERT)           ? "enabled" : "disabled");
    seq_printf(m , "dump tx skb      - %s\n", (g_dbg_enable & DBG_ENABLE_MASK_DUMP_SKB_TX)      ? "enabled" : "disabled");

#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
    seq_printf(m , "\n");
    seq_printf(m , "Current expected vector timer(ms): %u\n", vector_timer_interval);
    seq_printf(m , "Vector tx API is %s\n", en_vector_tx_api?"Enabled":"Disabled");
    seq_printf(m , "Vector tx API is in %s\n", en_vector_tx_api_test?"Test mode":"Working mode");
    seq_printf(m , "AVM mei_dsl_cb_func call cnt %u\n", avm_mei_dsm_cb_func_call_cnt);
    seq_printf(m , "AVM mei_dsl_cb_func call too fast %u\n", avm_mei_dsm_cb_func_call_too_fast);
    seq_printf(m , "AVM mei_dsl_cb_func mean time between func calls=%llu \n", avm_mean_time);

    seq_printf(m , "avm_in_irq %u\n", avm_in_irq);
    seq_printf(m , "avm_in_interrupt %u\n", avm_in_interrupt);
    seq_printf(m , "avm_irqs_disabled %u\n", avm_irqs_disabled);
    seq_printf(m , "avm_irqs_queue_cnt %u\n", avm_irqs_queue_cnt);
    seq_printf(m , "avm_irqs_tasklet_dequeue %u\n", avm_tasklet_dequeue_cnt);

#endif /*CONFIG_IFX_VECTOR_TIMER_CHECK */

#if defined(VECTOR_QUEUE_XMIT)
    seq_printf(m , "VECTOR_QUEUE_XMIT enabled\n");
#else
    seq_printf(m , "VECTOR_QUEUE_XMIT disabled\n");
#endif

    return 0;
}

static int proc_write_dbg(struct file *file, const char __user *buf, size_t count, loff_t *data)
{

    static const char *dbg_enable_mask_str[] = {
        " err",
        " error print",
        " dbg",
        " debug print",
        " assert",
        " assert",
        " tx",
        " dump tx skb",
        " all"
    };
    static const int dbg_enable_mask_str_len[] = {
        4, 12,
        4, 12,
        7,  7,
        3, 12,
        4
    };
    u32 dbg_enable_mask[] = {
        DBG_ENABLE_MASK_ERR,
        DBG_ENABLE_MASK_DEBUG_PRINT,
        DBG_ENABLE_MASK_ASSERT,
        DBG_ENABLE_MASK_DUMP_SKB_TX,
        DBG_ENABLE_MASK_ALL
    };

    char str[2048];
    char *p;

    int len, rlen;

    int f_enable = 0;
    int i;

    len = count < sizeof(str) ? count : sizeof(str) - 1;
    rlen = len - copy_from_user(str, buf, len);
    while ( rlen && str[rlen - 1] <= ' ' )
        rlen--;
    str[rlen] = 0;
    for ( p = str; *p && *p <= ' '; p++, rlen-- );
    if ( !*p )
        return 0;

    if ( strncasecmp(p, "enable", 6) == 0 )
    {
        p += 6;
        f_enable = 1;
    }
    else if ( strncasecmp(p, "disable", 7) == 0 )
    {
        p += 7;
        f_enable = -1;
    }
    else if ( strncasecmp(p, "help", 4) == 0 || *p == '?' )
    {
        printk("echo <enable/disable> [");
        for ( i = 0; i < ARRAY_SIZE(dbg_enable_mask_str); i += 2 )
        {
            if ( i != 0 )
                printk("/%s", dbg_enable_mask_str[i] + 1);
            else
                printk(dbg_enable_mask_str[i] + 1);
        }
        printk("] > /proc/driver/vectoring\n");
#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
        printk("echo set_timer value: to adjust vector timer\n");
        printk("echo set_api 0 / 1: to enable/disablet vector tx API\n");
        printk("echo test_api sleep_in_ms: to test vector tx API after delay x ms\n");
#endif /*CONFIG_IFX_VECTOR_TIMER_CHECK */
    }
#if defined(CONFIG_IFX_VECTOR_TIMER_CHECK) && CONFIG_IFX_VECTOR_TIMER_CHECK
    else if ( strncasecmp(p, "set_timer", 9) == 0 )
    {
        p += 9;
        rlen -= 9;
        ignore_space( &p, &rlen);
        if ( rlen > 0 )
        {
            printk("last timer=%u\n", vector_timer_interval);
            vector_timer_interval = get_number(&p, &rlen, 0);
            printk("new  timer=%u\n", vector_timer_interval);
        }
        return count;
    }
    else if ( strncasecmp(p, "set_api", 7) == 0 )
    {
        p += 7;
        rlen -= 7;
        ignore_space( &p, &rlen);
        if ( rlen > 0 )
        {
            en_vector_tx_api = get_number(&p, &rlen, 0);
            printk("Vectoring Tx API is %s\n", en_vector_tx_api?"Enabled":"Disabled");
        }
        return count;
    }
    else if ( strncasecmp(p, "test_api", 8) == 0 )
    {
        unsigned int delay_ms=0;
        p += 8;
        rlen -= 8;
        ignore_space( &p, &rlen);
        if ( rlen > 0 )
        {
            delay_ms = get_number(&p, &rlen, 0);
            en_vector_tx_api_test=1;
            mei_dsm_cb_func(NULL);
            msleep(delay_ms);
            mei_dsm_cb_func(NULL);
            en_vector_tx_api_test=0;
            printk("Vector Tx API has been called two times with msleep %u ms\n", delay_ms );
        }

        return count;
    }
#endif /*CONFIG_IFX_VECTOR_TIMER_CHECK */
    if ( f_enable )
    {
        if ( *p == 0 )
        {
            if ( f_enable > 0 )
                g_dbg_enable |= DBG_ENABLE_MASK_ALL;
            else
                g_dbg_enable &= ~DBG_ENABLE_MASK_ALL;
        }
        else
        {
            do
            {
                for ( i = 0; i < ARRAY_SIZE(dbg_enable_mask_str); i++ )
                    if ( strncasecmp(p, dbg_enable_mask_str[i], dbg_enable_mask_str_len[i]) == 0 )
                    {
                        if ( f_enable > 0 )
                            g_dbg_enable |= dbg_enable_mask[i >> 1];
                        else
                            g_dbg_enable &= ~dbg_enable_mask[i >> 1];
                        p += dbg_enable_mask_str_len[i];
                        break;
                    }
            } while ( i < ARRAY_SIZE(dbg_enable_mask_str) );
        }
    }

    return count;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
static struct file_operations g_proc_file_vectoring_dbg_seq_fops = {
    .owner      = THIS_MODULE,
    .open       = proc_read_dbg_seq_open,
    .read       = seq_read,
    .write      = proc_write_dbg,
    .llseek     = seq_lseek,
    .release    = single_release,
};
#else
static struct proc_ops g_proc_file_vectoring_dbg_seq_fops = {
    .proc_open    = proc_read_dbg_seq_open,
    .proc_read    = seq_read,
    .proc_write   = proc_write_dbg,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
#endif

static int proc_read_dbg_seq_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_dbg, NULL);
}

static void dump_data(void *data, unsigned int len, char *title)
{
    unsigned int i;

    if ( title )
        printk("%s\n", title);
    for ( i = 1; i <= len; i++ )
    {
        if ( i % 16 == 1 )
            printk("  %4d:", i - 1);
        printk(" %02X", (int)(*((char*)data + i - 1) & 0xFF));
        if ( i % 16 == 0 )
            printk("\n");
    }
    if ( (i - 1) % 16 != 0 )
        printk("\n");
}

static void dump_skb(struct sk_buff *skb, u32 len, char *title, int port, int ch, int is_tx, int enforce)
{
    if ( !enforce && !(g_dbg_enable & (is_tx ? DBG_ENABLE_MASK_DUMP_SKB_TX : 0)) )
        return;

    if ( skb->len < len )
        len = skb->len;

    if ( len > DMA_PACKET_SIZE )
    {
        printk("too big data length: skb = %08x, skb->data = %08x, skb->len = %d\n", (u32)skb, (u32)skb->data, skb->len);
        return;
    }

    if ( ch >= 0 )
        printk("%s (port %d, ch %d)\n", title, port, ch);
    else
        printk("%s\n", title);
    printk("  skb->data = %08X, skb->tail = %08X, skb->len = %d\n", (u32)skb->data, (u32)skb->tail, (int)skb->len);
    dump_data(skb->data, len, NULL);
}



/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

static int __init vectoring_init(void)
{

#if defined(VECTOR_QUEUE_XMIT)
    skb_queue_head_init(&vector_irq_queue);
    tasklet_init(&vector_irq_tasklet, vector_irq_tasklet_fn, 0);
#endif

    proc_create("driver/vectoring",
                S_IRUGO|S_IWUSR,
                0,
                &g_proc_file_vectoring_dbg_seq_fops);

    register_netdev_event_handler();
    g_ptm_net_dev = dev_get_by_name(&init_net, "dsl0");
    if ( g_ptm_net_dev != NULL )
        dev_put(g_ptm_net_dev);

    mei_dsm_cb_func_hook = mei_dsm_cb_func;

    return 0;
}

static void __exit vectoring_exit(void)
{
    mei_dsm_cb_func_hook = NULL;

    unregister_netdev_event_handler();

    remove_proc_entry("driver/vectoring", NULL);
}

module_init(vectoring_init);
module_exit(vectoring_exit);

MODULE_LICENSE("GPL");
