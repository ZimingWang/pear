##Pear Database
[中文版 README](./README.md)

###This is an upgrade of [Up Database](http://www.github.com/UncP/Up_Database)

###Current Version 0.1.0

####Goals
- Faster than Up Database at million level data insertion and deletion
- Module
- Implement the feature of ACID

####Features
- More simplified syntax [pear syntax](./pear_syntax)


###Version Information
#####Test Data 1000000 tuples, 80 bytes/tuple
	Consist of
		- key    16  bytes
		- value  64  bytes (1 key 1 value)

* Version 0.1.0
	- Insertion speed			``` 328000 tuples/second ```
	- Deletion speed			``` 204000 tuples/second ```


###TODO
- [ ] insert, select, delete operation