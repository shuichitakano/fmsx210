/*
 * author : Shuichi TAKANO
 * since  : Sun Sep 08 2019 13:33:0
 */
#ifndef B8E6F323_8134_16B4_CB67_1557554CB380
#define B8E6F323_8134_16B4_CB67_1557554CB380

#include <assert.h>
#include <experimental/optional>
#include <string>
#include <stdio.h>
#include <vector>
#include <sd_card/ff.h>

#define PICOJSON_ASSERT(x) assert(x)
#define PICOJSON_OVERFLOW(str) assert(!str)
#include <picojson.h>

inline std::experimental::optional<picojson::value>
parseJSONFile(const std::string &filename)
{
#if 0
    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp)
    {
        printf("JSON file read failed.\n");
        return {};
    }
    fseek(fp, SEEK_END, 0);
    std::vector<uint8_t> buffer(ftell(fp));
    fseek(fp, SEEK_SET, 0);
    fread(buffer.data(), buffer.size(), 1, fp);
    fclose(fp);
#else
    FIL f;
    if (f_open(&f, filename.c_str(), FA_READ) != FR_OK)
    {
        printf("JSON file read failed.\n");
        return {};
    }
    std::vector<uint8_t> buffer(f_size(&f));
    UINT br;
    f_read(&f, buffer.data(), buffer.size(), &br);
    f_close(&f);
#endif

    picojson::value root;
    std::string err;
    parse(root, buffer.begin(), buffer.end(), &err);
    if (!err.empty())
    {
        printf("JSON file parse error. (%s)\n", err.c_str());
        return {};
    }
    return root;
}

inline std::experimental::optional<picojson::value>
get(const picojson::value &obj, const char *name)
{
    auto v = obj.get(name);
    if (v.is<picojson::null>())
    {
        return {};
    }
    return v;
}

template <class T>
inline std::experimental::optional<T>
get(const picojson::value &v)
{
    if (v.is<T>())
    {
        return v.get<T>();
    }
    return {};
}

template <class T>
inline std::experimental::optional<T>
getValue(const picojson::value &obj, const char *name)
{
    if (auto v = get(obj, name))
    {
        if (auto val = get<T>(*v))
        {
            return val;
        }
        else
        {
            printf("invalid JSON value '%s'\n", name);
        }
    }
    return {};
}

template <class T>
inline T
getValue(const picojson::value &obj, const char *name, T default_val)
{
    if (auto v = getValue<T>(obj, name))
    {
        return *v;
    }
    return default_val;
}

inline std::experimental::optional<int>
getInt(const picojson::value &obj, const char *name)
{
    if (auto v = getValue<double>(obj, name))
    {
        return (int)*v;
    }
    return {};
}

inline int
getInt(const picojson::value &obj, const char *name, int default_val)
{
    if (auto v = getInt(obj, name))
    {
        return *v;
    }
    return default_val;
}

template <>
inline std::experimental::optional<int>
getValue<int>(const picojson::value &obj, const char *name)
{
    return getInt(obj, name);
}

#endif /* B8E6F323_8134_16B4_CB67_1557554CB380 */
