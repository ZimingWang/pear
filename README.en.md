##Pear Database
[中文版 README](./README.md)

###This is an upgrade of [Up Database](http://www.github.com/UncP/Up_Database)
####If you are implementing a database yourself, or you are new to database learning, it is highly recommended that you open directory [v0.1.0](./v0.1.0)

###Current Version 0.1.1

####Goals
- Faster than Up Database at million level data insertion and deletion
- Module, easy-to-extend functionality
- Implement the feature of ACID

####Features
- More simplified syntax [pear syntax](./pear_syntax)
- More excellent speed

###Version Information
####Test Data 1000000 tuples, 80 bytes/tuple
	Consist of: key  16  bytes,  value  64  bytes

* Version 0.1.0
	- single-thread insert and delete
		+ Insertion speed			``` 328000 tuples/second ```
		+ Deletion speed			``` 204000 tuples/second ```

* Version 0.1.0
	- implement thread pool that only use a few fixed memory(work queue uses remap to insure the job position that is freed earlier will be assigned a next job earlier)
	- fake multi-thread insertion and deletion

###TODO
- [x] insert, select, delete operation
- [x] thread pool
- [x] lock manager
- [ ] concurrent index
