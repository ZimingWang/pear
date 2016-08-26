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
	token->ele = calloc(MAX_ATTRIBUTE, sizeof(uint8_t *));
	if (!token->ele) fatal("Token初始化失败 :(");
	token->len = calloc(MAX_ATTRIBUTE, sizeof(uint16_t));
	if (!token->len) fatal("Token初始化失败 :(");
	return Ok;
}

status init_parser()
{
	parser.db = NULL;
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
	if (parser.db)
		shut(parser.db);
	_free_buffer(&parser.buffer);
	_free_token(&parser.token);
}

static status _next_buffer()
{
	parser.processed += parser.buffer.len;
	parser.buffer.len = pread64(parser.fd, parser.buffer.mem, BUFFER_SIZE, parser.processed);
	if (parser.buffer.len > 0) {
		parser.buffer.ptr = parser.buffer.mem;
		char *ptr = (char *)(parser.buffer.mem + parser.buffer.len - 1);
		memset(parser.buffer.mem + parser.buffer.len, 0, 8);
		for (; *ptr != '\n' && *ptr != '\0';) {
			*ptr-- = '\0';
			--parser.buffer.len;
		}
		return Ok;
	} else if (!parser.buffer.len) {
		return Bad;
	} else {
		fatal("pear文件读取失败 :(");
	}
}

static bool _parse_put(char **s, Token *tk)
{
	char *ptr = ++*s;
	for (; *ptr != '\n' && *ptr != '\0'; ) {
		if (*ptr != '\'') return false;
		tk->ele[tk->count] = ++ptr;
		char *new = strchr(ptr, '\'');
		*new = '\0';
		tk->len[tk->count] = new - ptr;
		ptr = ++new;
		++tk->count;
		// 到达这里时, 指针值应该为 ' ' 或 '\n' 或 '\0'
		if (*ptr == ' ') ++ptr;
	}
	*s = ptr;
	return true;
}

static bool _parse_key(char **s, Token *tk)
{
	char *ptr = ++*s;
	if (*ptr != '\'') return false;
	tk->ele[0] = ++ptr;
	char *new = strchr(ptr, '\'');
	tk->len[0] = new - ptr;
	ptr = new;
	*ptr++ = '\0';
	if (*ptr != '\n' && *ptr != '\0') return false;
	*s = ptr;
	return true;
}

static bool _parse_database(char **s, Token *tk)
{
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
		char *new = ptr;
		while (*ptr != ' ' && *ptr != '\n') ++ptr;
		*ptr++ = '\0';
		if (!strcmp(new, "str")) {
			while (*ptr == ' ') ++ptr;
			tk->len[tk->count] = STR;
			while (isdigit(*ptr))
				tk->len[tk->count] = 10 * tk->len[tk->count] + (*ptr++ - '0');
			if (*ptr++ != '\n') return false;
		} else {
			return false;
		}
		++tk->count;
		++parser.line;
	}
	if (*++ptr != '\n') return false;
	++parser.line;
	*s = ptr;
	return true;
}

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

static void _reset_buffer(Buffer *buffer)
{
	buffer->len = 0;
	buffer->ptr = buffer->mem;
	memset(buffer->mem, 0, BUFFER_SIZE);
}

static void _reset_parser()
{
	parser.processed = 0;
	parser.line = 0;
	_reset_buffer(&parser.buffer);
}

status parse(const char *file)
{
	if ((parser.fd = open(file, O_RDONLY)) < 0) warning("无法打开 %s :(", file);
	_reset_parser();
	status s;
	for (;;) {
		if ((s = _parse_line()) != Ok) {
			close(parser.fd);
			if (parser.db) printf("%d\n", parser.db->tuple);
			return s;
		}
		switch(parser.token.op) {
			case PUT:
				if (!parser.db)
					alert("未选定或创建数据库 :(");
				else
					put(parser.db, parser.token.ele, parser.token.len, parser.token.count);
				break;
			case DROP:
				if (!parser.db)
					alert("未选定或创建数据库 :(");
				else
					drop(parser.db, parser.token.ele[0], parser.token.len[0]);
				break;
			case GET:
				if (!parser.db)
					alert("未选定或创建数据库 :(");
				else
					get(parser.db, parser.token.ele[0], parser.token.len[0]);
				break;
			case OPEN:
				if (!parser.db)
					parser.db = newDB((const char *)parser.token.ele[0]);
				else
					warning("存在打开的数据库 %s :(", parser.db->name);
				break;
			case SHUT:
				if (parser.db) {
					if (!strncmp(parser.db->name, (char *)parser.token.ele[0], parser.token.len[0])) {
						if (shut(parser.db)) parser.db = NULL;
					} else {
						warning("数据库名称错误 %s :(", (char *)parser.token.ele[0]);
					}
				} else
					warning("不存在打开的数据库 %s :(", (char *)parser.token.ele[0]);
				break;
			case REMOVE:
				warning("not implemented :(");
				break;
			case NEW:
				if (parser.db)
					create_table(parser.db, parser.token.ele, parser.token.len, parser.token.count);
				else
					alert("未选定或创建数据库 :(");
				break;
		}
	}
}
