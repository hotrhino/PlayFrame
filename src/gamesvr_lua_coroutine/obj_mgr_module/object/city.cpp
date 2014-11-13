/**
 * @file city.cpp
 * @brief City对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "city.h"

void City::SetCityByDb(const Database::CityInfo* city_info)
{
    population_ = city_info->population();
}

