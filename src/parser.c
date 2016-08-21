/**
 *    > Author:   UncP
 *    > Mail:     770778010@qq.com
 *    > Github:   https://www.github.com/UncP/pear
 *    > Description:
 *
 *    > Created Time: 2016-08-20 19:56:47
**/

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "table.h"

enum OP { PUT, DROP, GET, OPEN, SHUT, REMOVE, NEW };

static Parser parser;

static status _init_buffer(Buffer *buffer)
{
	buffer->mem = calloc(BUFFER_SIZE, sizeof(uint8_t));
	if (!buffer->mem) fatal("缓冲区初始化失败 :(");
	buffer->len = 0;
	buffer->ptr = buffer->mem;
	return Ok;
}

static status _init_token(Token *token)
{
	token->ele = calloc(MAX_ATTRIBUTE_SIZE, sizeof(uint8_t *));
	if (!token->ele) fatal("Token初始化失败 :(");
	token->len = calloc(MAX_ATTRIBUTE_SIZE, sizeof(uint16_t));
	if (!token->len) fatal("Token初始化失败 :(");
	return Ok;
}

status init_parser()
{
	if (_init_buffer(&parser.buffer) == Fatal) return Fatal;
	if (_init_token(&parser.token) == Fatal) return Fatal;
	parser.fd 	= -1;
	parser.processed = 0;
	parser.line = 0;
	return Ok;
}

void _free_buffer(Buffer *buffer)
{
	if (buffer->mem) free(buffer->mem);
}

void _free_token(Token *token)
{
	if (token->ele) free(token->ele);
	if (token->len) free(token->len);
}

void free_parser()
{
	_free_buffer(&parser.buffer);
	_free_token(&parser.token);
}

static status _next_buffer()
{
	parser.processed += parser.buffer.len;
	parser.buffer.len = pread(parser.fd, parser.buffer.mem, BUFFER_SIZE, parser.processed);
	if (parser.buffer.len > 0) {
		parser.buffer.ptr = parser.buffer.mem;
		char *ptr = (char *)(parser.buffer.mem + parser.buffer.len - 1);
		memset(parser.buffer.mem + parser.buffer.len, 0, BUFFER_SIZE - parser.buffer.len);
		for (; *ptr != '\n' && *ptr != '\0';) {
			*ptr-- = '\0';
			--parser.buffer.len;
		}
		return Ok;
	} else if (!parser.buffer.len) {
		return Warning;
	} else {
		fatal("pear文件读取失败 :(");
	}
}

static bool _parse_put(char **s, Token *tk)
{// 获取所有要插入的元素,并依次使指针指向这些元素以备验证, 若是字符串, 记录下字符串长度
 // 当函数返回时, s 指向 '\n'
	char *ptr = ++*s;
	for (; *ptr != '\n' && *ptr != '\0'; ) {
		switch(*ptr) {
		case '\'':
			tk->ele[tk->count] = (void *)++ptr;
			char *new = strchr(ptr, '\'');
			*new = '\0';
			tk->len[tk->count] = (uint16_t)(new - ptr);
			ptr = ++new;
			++tk->count;
			break;
		default:
			return false;
		}
		// 到达这里时, 指针值应该为 ' ' 或 '\n' 或 '\0'
		if (*ptr == ' ') ++ptr;
	}
	*s = ptr;
	return true;
}

static bool _parse_key(char **s, Token *tk)
{// 获取关键值, 目前只支持字符串作关键值
 // 当函数返回时, s 指向 '\n'
	char *ptr = ++*s;
	if (*ptr != '\'') return false;
	tk->ele[0] = ++ptr;
	ptr = strchr(ptr, '\'');
	*ptr++ = '\0';
	// 到达这里时, 指针值应该为 '\n' 或 '\0'
	if (*ptr != '\n' && *ptr != '\0') return false;
	*s = ptr;
	return true;
}

static bool _parse_database(char **s, Token *tk)
{// 获取 'database' 及数据库名
 // 当函数返回时, s 指向 '\n'
	char *ptr = ++*s;
	char *new = strchr(ptr, ' ');
	*new = '\0';
	if (strcmp(ptr, "database")) return false;
	ptr = ++new;
	while (*ptr == ' ') ++ptr;
	tk->ele[0] = ptr;
	ptr = strchr(ptr, '\n');
	*ptr = '\0';
	*s = ptr;
	return true;
}

static bool _parse_table(char **s, Token *tk)
{// 获取属性信息
 // 当函数返回时, s 指向 '\n'
	char *ptr = ++*s;
	char *new = strchr(ptr, ' ');
	*new = '\0';
	if (strcmp(ptr, "table")) return false;
	ptr = ++new;
	if (*ptr++ != '[') return false;
	if (*ptr++ != '\n') return false;
	++parser.line;
	for (; *ptr != ']';) {
		while (*ptr == ' ') ++ptr;
		tk->ele[tk->count] = ptr;
		ptr = strchr(ptr, ' ');
		*ptr++ = '\0';
		while (*ptr == ' ') ++ptr;
		char *new = strchr(ptr, ' ');
		*new = '\0';
		if (!strcmp(ptr, "str")) {
			ptr = ++new;
			while (*ptr == ' ') ++ptr;
			tk->len[tk->count] = 0;
			while (isdigit(*ptr))
				tk->len[tk->count] = 10 * tk->len[tk->count] + (*ptr++ - '0');
		} else {
			return false;
		}
		++tk->count;
		if (*ptr != '\n') return false;
		++parser.line;
		++ptr;
	}
	if (*++ptr != '\n') return false;
	++parser.line;
	*s = ptr;
	return true;
}

/**
 *		open database  `name`
 *
 *		shut database  `name`
 *
 *		remove database  `name`
 *
 *		new table [
 *		  `name`  `type`
 *		  `name`  `type`
 *		  ...
 *		]
 *
 *		put  key  val val ...
 *
 *		get  key
 *
 *		drop  key
**/

static status _parse_line()
{// 解析一行 pear 语句, 解析只负责语法检查, 不负责语义检查
	if (!(*(char *)parser.buffer.ptr)) {
		status s = _next_buffer();
		if (s != Ok) return s;
	}
	char *ptr = (char *)parser.buffer.ptr;
	Token *tk = &parser.token;
	tk->count = 0;
	for (; *ptr;) {
		++parser.line;
		switch(*ptr) {
		case 'p':
			if (*++ptr == 'u' && *++ptr == 't' && *++ptr == ' ' && _parse_put(&ptr, tk)) {
				tk->op = PUT;
				goto done;
			}
			break;
		case 'd':
			if (*++ptr == 'r' && *++ptr == 'o' && *++ptr == 'p' && *++ptr == ' ' &&
					_parse_key(&ptr, tk)) {
				tk->op = DROP;
				goto done;
			}
			break;
		case 'g':
			if (*++ptr == 'e' && *++ptr == 't' && *++ptr == ' ' && _parse_key(&ptr, tk)) {
				tk->op = GET;
				goto done;
			}
			break;
		case 'o':
			if (*++ptr == 'p' && *++ptr == 'e' && *++ptr == 'n' && *++ptr == ' ' &&
					_parse_database(&ptr, tk)) {
				tk->op = OPEN;
				goto done;
			}
			break;
		case 's':
			if (*++ptr == 'h' && *++ptr == 'u' && *++ptr == 't' && *++ptr == ' ' &&
					_parse_database(&ptr, tk)) {
				tk->op = SHUT;
				goto done;
			}
			break;
		case 'r':
			if (*++ptr == 'e' && *++ptr == 'm' && *++ptr == 'o' && *++ptr == 'v'
			 && *++ptr == 'e' && *++ptr == ' ' && _parse_database(&ptr, tk)) {
				tk->op = REMOVE;
				goto done;
			}
			break;
		case 'n':
			if (*++ptr == 'e' && *++ptr == 'w' && *++ptr == ' ' && _parse_table(&ptr, tk)) {
				tk->op = NEW;
				goto done;
			}
			break;
		case '#':
			while (*ptr && *ptr != '\n') ++ptr;
		case '\n':
			++ptr;
			continue;
		}
		warning("第 %d 行 pear 语法错误 :(", parser.line);
	}
	return Ok;
done:
	// 缓冲区指针指向下一行
	parser.buffer.ptr = ++ptr;
	return Ok;
}
/*
static void _print_token(Token *token)
{
	if (!token->count) {
		printf("op %d   obj %s\n", token->op, (char *)token->ele[0]);
	} else {
		printf("op %d\n", token->op);
		for (uint8_t i = 0; i != token->count; ++i) {
			printf("%s  %d\n", (char *)token->ele[i], token->len[i]);
		}
	}
}
*/
status parse(const char *file)
{
	if ((parser.fd = open(file, O_RDONLY)) < 0) warning("无法打开 %s :(", file);
	status s;
	for (;;) {
		if ((s = _parse_line()) != Ok) return s;
		switch(parser.token.op) {
			case PUT:
			case DROP:
			case GET:
			case OPEN:
			case SHUT:
			case REMOVE:
			case NEW:
				break;
		}
	}
}
