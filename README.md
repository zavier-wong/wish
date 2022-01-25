# wish
a simple shell interpreter in c++. 用c++写的简单shell解释器

实现了管道和重定向，支持作业管理包括作业暂停、前台，后台运行调度

实现了shell常用的内建命令：alias，bg，cd，echo，exit，fg，jobs，kill，pwd， quit，type

## 构建/build
``` 
git clone https://github.com/zavier-wong/wish
cd wish
mkdir build
cmake ..
make
```
Built target in wish/build/wish

now you can run ./wish
