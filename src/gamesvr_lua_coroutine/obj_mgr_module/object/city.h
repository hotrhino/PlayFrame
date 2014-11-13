/**
 * @file city.h
 * @brief City对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CITY_H_
#define _CITY_H_

#include "app_def.h"

class City
{
    public:
        friend class ObjMgrModule;
        friend class Player;

        City() {}
        ~City() {}

        void init() { memset(this, 0, sizeof(*this)); }

        void set_player_idx(int32_t player_idx) { player_idx_ = player_idx; }
        int32_t player_idx() const { return player_idx_; }

        // 通过protobuf描述的blob数据初始化city
        void SetCityByDb(const Database::CityInfo* city_info);

        void set_population(int32_t population) { population_ = population; }
        int32_t population() const { return population_; }

    private:
        int32_t         player_idx_;

        int32_t         population_;
};

#endif 
