# linux-kernel-debug

proclist.c

A driver to list running processes in kernel mode


# Install
```
make
sudo insmod proclist.ko
sudo cat /sys/kernel/debug/proclist
```

# Verify
```
sudo dmesh|tail
```

# Remove
```
sudo rmmod proclist.ko
```
