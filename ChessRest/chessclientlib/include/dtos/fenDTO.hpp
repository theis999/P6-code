#pragma once

#ifndef SMAK_FENDTO_HPP
#define SMAK_FENDTO_HPP

#include <oatpp/core/Types.hpp>
#include <oatpp/core/macro/codegen.hpp>
#include <chess.hpp>

namespace smak::models {
    
#include OATPP_CODEGEN_BEGIN(DTO)

class FenDTO : public oatpp::DTO {
    DTO_INIT(FenDTO, DTO)

    DTO_FIELD(Int64, id);
    DTO_FIELD(Int32, ply);
    //DTO_FIELD(Enum<GameResultEnum>, result);
    //DTO_FIELD(Enum<GameResultReasonEnum>, reason);

    DTO_FIELD(String, fen_string);

    DTO_FIELD_INFO(move) {
        info->required = false;
    }
    DTO_FIELD(String, move) ;

};


#include OATPP_CODEGEN_END(DTO)


}

#endif
