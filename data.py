import random, os

def init():
	dst = open('0.pear', 'w')
	dst.write('''open database my

new table [
  id    str   16
  name  str   64
]\n''')

def generate_data(num, file1, file2):
	choice = 'abcdefghijklmnopqrstuvwxyzABCDEFG1234567890-'
	index  = len(choice) - 1
	dst1  = open(file1, 'w')
	dst2 = open(file2, 'w')
	for i in range(0, num):
		dst1.write('put ')
		id = ''
		for j in range(0, 15):
			id += choice[random.randint(0, index)]
		dst1.write('\'' + id + '\' ')
		name = ''
		for j in range(0, 63):
			name += choice[random.randint(0, index)]
		dst1.write('\'' + name + '\'' + '\n')
		dst2.write('drop \'' + id + '\'' + '\n')
	dst1.close()
	dst2.close()

if __name__ == '__main__':
	if not os.path.exists('pear_syn'):
		os.mkdir('pear_syn')
	os.chdir('pear_syn')
	init()
	num = 10
	num += 1
	for i in range(1, num):
		file1 = str(i) + '.pear'
		file2 = str(num - 1 + i) + '.pear'
		generate_data(10000, file1, file2)
