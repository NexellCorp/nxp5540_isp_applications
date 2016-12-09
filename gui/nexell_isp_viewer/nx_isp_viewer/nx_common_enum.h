#ifndef NXCOMMONENUM_H
#define NXCOMMONENUM_H

#define MAX_BUFFER_COUNT	32

struct nx_device {
    /* drm information */
    int drm_fd;
    int gem_fds[MAX_BUFFER_COUNT];
    int dma_fds[MAX_BUFFER_COUNT];
    void *pVaddrs[MAX_BUFFER_COUNT];

    /* queue */
    unsigned int dq_index;
    unsigned int buffer_count;
    /* vidoe frame inforamtion */
    int video_fd;
    unsigned int format;
    unsigned int buf_type;
    unsigned int mem_type;
    unsigned int width;
    unsigned int height;
    unsigned int size;
};

enum nx_cmd_list {
    nx_cmd_default = 0,
    nx_cmd_open_device,
    nx_cmd_dump1,
    nx_cmd_control_reg,
    nx_cmd_load_sensor_regset,
    nx_cmd_load_isp_regset,
    //nx_cmd_save_cur_reg,
    nx_cmd_preview_start,
    nx_cmd_dump2,
    nx_cmd_preview_stop,
    nx_cmd_close_device,
    nx_cmd_exit,
};

enum viewer_status{
    nx_viewer_main = 0,
    nx_viewer_connecting,
    nx_viewer_connected,
    nx_viewer_preview_start,
    nx_viewer_previewing,
    nx_viewer_preview_stop,
    nx_viewer_disconnecting,
    nx_viewer_exit,
};

struct nx_image_format {
    unsigned int width;
    unsigned int height;
};

enum nx_reg_flag {
    nx_reg_flag_read = 0,
    nx_reg_flag_write,
    nx_reg_flag_read_page,
    nx_reg_flag_write_page,
};

enum nx_reg_type {
    nx_reg_type_isp = 0,
    nx_reg_type_sensor,
};

struct nx_reg_control {
    nx_reg_type    type;
    nx_reg_flag    flag;
    unsigned int    addr;
    void* data;
};

#endif // NXCOMMONENUM_H
