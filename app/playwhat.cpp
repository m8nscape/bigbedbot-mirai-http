#include <cstdlib>
#include <cstring>

#include "playwhat.h"

#include "utils/logger.h"
#include "utils/rand.h"

#include <curl/curl.h>

namespace playwhat
{

class curl_buffer
{
public:
    int length = 0;
    char* content = NULL;
    curl_buffer() = delete;
    curl_buffer(int len) { content = new char[len]; memset(content, 0, len * sizeof(char)); }
    ~curl_buffer() { if (content) delete content; }
};

size_t curl_write(void* buffer, size_t size, size_t count, void* stream)
{
    curl_buffer* buf = (curl_buffer*)stream;
    int newsize = size * count;
    memcpy(buf->content + buf->length, buffer, newsize);
    buf->length += newsize;
    return newsize;
}

///////////////////////////////////////////////////////////////////////////////

int SteamAppListParser::parse(char* s)
{
    if (!s) return 1;
    games.clear();
    c_pointer = s;

    Estat prev_stat = Estat::IDLE;
    int stat_counter = 0;

    while (c_pointer && (*c_pointer) != 0)
    {
        if (prev_stat == stat)
        {
            ++stat_counter;
            if (stat_counter > 512)
            {
                return 5;
            }
        }
        else
        {
            stat_counter = 0;
        }

        prev_stat = stat;
        switch (stat)
        {
        case Estat::IDLE:
            proc_IDLE();
            break;

        case Estat::KEY_LEFT:
            proc_KEY_LEFT();
            break;

        case Estat::KEY_QUOTE:
            proc_KEY_QUOTE();
            break;

        case Estat::KEY_RIGHT:
            proc_KEY_RIGHT();
            break;

        case Estat::VALUE_LEFT:
            proc_VALUE_LEFT();
            break;

        case Estat::VALUE_NUM:
            proc_VALUE_NUM();
            break;

        case Estat::VALUE_QUOTE:
            proc_VALUE_QUOTE();
            break;

        case Estat::VALUE_RIGHT:
            proc_VALUE_RIGHT();
            break;

        case Estat::PAIR_FINISH:
            proc_PAIR_FINISH();
            break;

        default:
            return 3;
        }
        if (brace_counter < 0)
            return 4;
    }

    if (c_pointer == NULL || brace_counter != 0 || stat != Estat::IDLE) return 2;

    available = true;
    return 0;
}

int SteamAppListParser::proc_IDLE()
{
    switch (*c_pointer)
    {
    case '{':
        stat = Estat::KEY_LEFT;
        ++brace_counter;
        break;

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_KEY_LEFT()
{
    switch (*c_pointer)
    {
    case '{':
    case '[':
        ++brace_counter;
        break;

    case '}':
        --brace_counter;
        break;

    case '"':
        stat = Estat::KEY_QUOTE;
        memset(key, 0, 256);
        kp = key;
        break;

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_KEY_QUOTE()
{
    switch (*c_pointer)
    {
    case '"':
        stat = Estat::KEY_RIGHT;
        break;

    case '\\':
        ++c_pointer;
    default:
        *kp = *c_pointer;
        ++kp;
        break;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_KEY_RIGHT()
{
    switch (*c_pointer)
    {
    case ':':
        stat = Estat::VALUE_LEFT;
        break;

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_VALUE_LEFT()
{
    switch (*c_pointer)
    {
    case '{':
    case '[':
        stat = Estat::KEY_LEFT;
        ++brace_counter;
        break;

    case '"':
        stat = Estat::VALUE_QUOTE;
        memset(value, 0, 256);
        vp = value;
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        stat = Estat::VALUE_NUM;
        memset(value, 0, 256);
        vp = value;
        return proc_VALUE_NUM();

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_VALUE_NUM()
{
    switch (*c_pointer)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        *vp = *c_pointer;
        ++vp;
        break;

    case ']':
    case '}':
        --brace_counter;
    case ',':
        stat = Estat::PAIR_FINISH;
        break;

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_VALUE_QUOTE()
{
    switch (*c_pointer)
    {
    case '"':
        stat = Estat::VALUE_RIGHT;
        break;

    case '\\':
        ++c_pointer;
    default:
        *vp = *c_pointer;
        ++vp;
        break;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_VALUE_RIGHT()
{
    switch (*c_pointer)
    {
    case ']':
    case '}':
        --brace_counter;
    case ',':
        stat = Estat::PAIR_FINISH;
        break;

    default:
        parse_fail();
        return 1;
    }
    ++c_pointer;
    return 0;
}

int SteamAppListParser::proc_PAIR_FINISH()
{
    if (!strcmp("appid", key))
    {
        appid_buf = atol(value);
    }
    else if (!strcmp("name", key) && appid_buf != 0)
    {
        game g;
        g.appid = appid_buf;
        //g.name = utf82gbk(value);
        g.name.shrink_to_fit();

		if (!(g.name.find("Demo") != g.name.npos ||
			g.name.find("Bundle") != g.name.npos ||
			g.name.find("Pack") != g.name.npos ||
			g.name.find("Trial") != g.name.npos ||
			g.name.find("DLC") != g.name.npos ||
			g.name.find("Downloadable Content") != g.name.npos ||
			g.name.find("Demo") != g.name.npos ||
			g.name.find("test") != g.name.npos ||
			g.name.find("OST") != g.name.npos ||
			g.name.find("Soundtrack") != g.name.npos))
		{
			games.push_back(std::move(g));
		}

        appid_buf = 0;
    }

    bool check = true;
    while (check)
    {
        switch (*c_pointer)
        {
        case ']':
        case '}':
            stat = Estat::KEY_LEFT;
            --brace_counter;
            ++c_pointer;
            break;

        case ',':
            ++c_pointer;
        case '"':
            stat = Estat::KEY_LEFT;
            check = false;
            break;

        case 0:
            stat = Estat::IDLE;
            check = false;
            break;

        default:
            parse_fail();
            return 1;
        }
    }
    return 0;
}

void SteamAppListParser::parse_fail()
{
    c_pointer = NULL;
}

///////////////////////////////////////////////////////////////////////////////

//std::vector<std::pair<long, std::string>> steamGameList;
SteamAppListParser games;
void updateSteamGameList()
{
#ifdef _DEBUG
    return;
#endif

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        addLog(LOG_WARNING, "play", "curl init error");
    }

    curl_buffer curlbuf(8 * 1024 * 1024);   // 8MB
    curl_easy_setopt(curl, CURLOPT_URL, "http://api.steampowered.com/ISteamApps/GetAppList/v0002/?format=json");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlbuf);
#ifdef _DEBUG
    //curl_easy_setopt(curl, CURLOPT_PROXY, "http://localhost:10800");
#endif
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    int ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    switch (ret)
    {
    case CURLE_OK:
	{
		SteamAppListParser gamestmp;
		switch (gamestmp.parse(curlbuf.content))
		{
		case 0:
			games.games.clear();
			games = std::move(gamestmp);
			char msg[128];
			sprintf_s(msg, "added %u games", games.games.size());
			addLogDebug("play", msg);
			break;

		case 2:
			addLog(LOG_WARNING, "play", "parse error");
			break;

		case 3:
			addLog(LOG_WARNING, "play", "parsing fsm state error");
			break;

		case 4:
			addLog(LOG_WARNING, "play", "unexpected end of brace");
			break;

		case 5:
			addLog(LOG_WARNING, "play", "deadloop detected");
			break;

		default:
			break;
		}
	}
		break;

    default:
        addLog(LOG_WARNING, "play", "curl error %d", ret);
        break;
    }

}


}