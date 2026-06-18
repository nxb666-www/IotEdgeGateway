class AdapterBase {
public:
    virtual ~AdapterBase() = default;
    virtual std::string Name() const = 0;  // 返回适配器名称
    virtual bool Start() = 0;              // 启动
    virtual void Stop() = 0;               // 停止
};