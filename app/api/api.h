
#include "utils/http_worker.hpp"

namespace bbb::api
{
class ApiHttpWorker: public http_worker
{
public:
    ApiHttpWorker(tcp::acceptor& acceptor) : http_worker(acceptor)
    {
    }

public:
    virtual bool process_head(boost::beast::string_view target, const std::string& body) override;
    virtual bool process_get(boost::beast::string_view target, const std::string& body) override;
    virtual bool process_post(boost::beast::string_view target, const std::string& body) override;
};

class ApiInstance
{
protected:
    std::string addr = "0.0.0.0";
    unsigned short port = 0;
    bool running = false;
    bool quit = false;

public:
    void set_port(unsigned short port);
    void start();
    void stop();

protected:
    static void start_server(std::string addr, unsigned short port, int num_workers, bool* quit, bool* running);
};
inline ApiInstance inst;

}