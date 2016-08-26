##pear Database
[English Version of README](./README.en.md)

###这是[Up Database](http://www.github.com/UncP/Up_Database)的升级版本

###当前版本 0.1.0

####目标
- 超过 Up Database 在百万级别数据的插入与删除速度
- 模块化, 高度可扩展
- 实现数据库的 ACID 特性

####特点
- 更简洁的语法 [pear syntax](./pear_syntax)
- 更优秀的性能


###版本信息
####测试数据 1000000 组, 每组 80 字节
	组成: 键 16 字节,  值 64 字节

* 版本 0.1.0
	- 插入性能			``` 328000 组/秒 ```
	- 删除性能			``` 204000 组/秒 ```


###TODO
- [ ] 插入, 查找, 删除操作



###Special Version 0.1.0
	采用B+树作为索引的非关系型数据库，纯C编写，一共6个模块，16个文件
	连头文件在内不到两千行．
	核心 *btree.c* 和 *page.c* 加起来不到 800 行，
	虽然它并不拥有数据库的ACID特性，但是它足够小，语法足够简单，
	性能足够优秀，并且支持千万级别的数据插入与删除．

	pear v0.1.0 可以让你快速了解数据库整体架构和具体实现．
	如果你对 pear 这个特殊版本感兴趣，
	git clone https://github.com/UncP/pear.git
	cd pear/src
	make && python ../generate_data.py && ./run
	一百万组测试数据采用python脚本生成, python 2.7 或 >=3.5 都可以，
	run 脚本可以修改程序运行参数，参数 *put* 代表要插入的数据数量，
	每个 put 代表10000组，参数 *drop* 代表要删除的数据数量．

	Enjoy !
