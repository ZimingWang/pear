##pear Database
[English Version of README](./README.en.md)

###这是[Up Database](http://www.github.com/UncP/Up_Database)的升级版本

###当前版本 0.1.0

####目标
- 超过 Up Database 在百万级别数据的插入与删除速度
- 模块化
- 实现数据库的 ACID 特性

####特点
- 更简洁的语法 [pear syntax](./pear_syntax)


###版本信息
####测试数据 1000000 组, 每组 80 字节
	组成
	- 键 16 字节
	- 值 64 字节 (1 键 1 值)

* 版本 0.1.0
	- 插入性能			``` 328000 组/秒 ```
	- 删除性能			``` 204000 组/秒 ```


###TODO
- [ ] 插入, 查找, 删除操作