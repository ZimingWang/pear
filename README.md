##pear Database
[English Version of README](./README.en.md)

###这是[Up Database](http://www.github.com/UncP/Up_Database)的升级版本
####如果你正在自己实现一个数据库，或者是数据库初学者, 强烈建议你打开文件夹 [v0.1.0](./v0.1.0)

###当前版本 0.1.1

####目标
- 超过 Up Database 在百万级别数据的插入与删除速度
- 模块化, 高度可扩展
- 实现数据库的 ACID 特性

####特点
- 更简洁的语法 [pear syntax](./pear_syntax)
- 更出色的性能


###版本信息
	测试数据 1000000 组, 每组 80 字节
	组成: 键 16 字节,  值 64 字节

* 版本 0.1.0
	- 单线程插入与删除
	- 性能
		+ 插入性能			``` 328000 组/秒 ```
		+ 删除性能			``` 204000 组/秒 ```

* 版本 0.1.1
	- 实现使用少量固定内存的线程池(工作队列采用二次映射来保证先结束工作的位置能够先获得下一次工作)
	- 伪多线程插入与删除


###TODO
- [x] 插入, 查找, 删除操作
- [x] 线程池
- [x] 锁管理器
- [ ] 并发索引
