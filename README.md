# mplayerRadio

#### 介绍
基于mplayer构建的网络电台收音程序,在128*64的小屏幕,树莓派Model A+上运行.
#### 运行原理

1. 先从这个仓库更新资源[https://gitee.com/wiselot/mplayer-radio-res.git](https://gitee.com/wiselot/mplayer-radio-res.git)(不知道gitee传文件容量多少,多的话放点下好的音乐)
2. 再以从模式启动mplayer,code:
```
execlp("mplayer","mplayer","-slave","-quiet","-idle","-input",fifo_buf,\
			radio_url_list[0],
			NULL);
```
3. 刷新两个面板,播放器页面和菜单选择页面,直到主动或被动退出或挂起

#### 安装
##### 要装的库
1. wiringPi : [https://github.com/WiringPi/WiringPi.git](https://github.com/WiringPi/WiringPi.git)
#### 编译&安装
1. bash setup.sh

#### 吐嘈
1. 为什么不用c++写,这样就不用去写一些不舒服的代码,比如strcpy,sprintf ~~因为一开始觉得没问题,后来懒得改~~
2. 为什么不用消息队列和信号量 ~~因为没怎么学过线程,呜呜~~
3. 为什么写那么多注释 **不是给别人看的,是防止以后回来继续写要花上更多时间去理解** 
4. 为什么gui那么丑 **因为128*64的oled不够发挥** ~~主要是懒~~ **要去看看大佬写的gui了,丝滑菜单切换**

#### 更新计划
**计划就是,先不更了,以后有心情再更,当然在6月后**
1. c++ 改写一遍
2. 消息队列和信号量
3. 美化gui
4. 完成歌曲信息展示
5. 更灵活的选择歌单,比如选本地文件夹中的list,音乐文件等
