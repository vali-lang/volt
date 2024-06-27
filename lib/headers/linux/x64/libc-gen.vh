
cstruct libc_timespec {
    tv_sec: i64
    tv_nsec: i64
}

cstruct libc_timeval {
    tv_sec: i64
    tv_usec: i64
}

cstruct libc_sockaddr {
    sa_family: i16
    sa_data: inline [i8, 14]
}

cstruct libc_pollfd {
    fd: i32
    events: i16
    revents: i16
}

cstruct __jmp_buf_tag {
    prop_0: inline [i64, 8]
    prop_1: i32
    prop_2: inline __sigset_t
}

cstruct __sigset_t {
    prop_0: inline [i64, 16]
}

cstruct libc_stat {
    st_dev: i64
    st_ino: i64
    st_nlink: i64
    st_mode: i32
    st_uid: i32
    st_gid: i32
    __pad0: i32
    st_rdev: i64
    st_size: i64
    st_blksize: i64
    st_blocks: i64
    st_atime: inline libc_timespec
    st_mtime: inline libc_timespec
    st_ctime: inline libc_timespec
    __unused: inline [i64, 3]
}

cstruct libc_dirent {
    d_ino: i64
    d_off: i64
    d_reclen: i16
    d_type: i8
    d_name: inline [i8, 256]
}

cstruct libc_timezone {
    tz_minuteswest: i32
    tz_dsttime: i32
}

cstruct libc_addrinfo {
    ai_flags: i32
    ai_family: i32
    ai_socktype: i32
    ai_protocol: i32
    ai_addrlen: i32
    ai_addr: ptr
    ai_canonname: ptr
    ai_next: ptr
}

cstruct libc_epoll_event packed {
    events: i32
    data: ptr
}

cstruct libc_jmp_buf {
    prop_0: inline [inline __jmp_buf_tag, 1]
}
