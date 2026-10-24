set pagination off
set confirm off
handle SIGPIPE nostop noprint
break get_physical_address if (addr >> 12) == 0xffffffffc0207
commands 1
  bt 8
  info args
  print/x env->satp
  print env->priv
  detach
  quit
end
continue
