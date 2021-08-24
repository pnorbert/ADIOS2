/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Serializer.h
 *
 */

#include "adios2/core/Attribute.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"

#include "BP5Deserializer.h"
#include "BP5Deserializer.tcc"

#include <limits.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace adios2
{
namespace format
{
void BP5Deserializer::InstallMetaMetaData(MetaMetaInfoBlock &MM)
{
    char *FormatID = (char *)malloc(MM.MetaMetaIDLen);
    char *MetaMetaInfo = (char *)malloc(MM.MetaMetaInfoLen);
    memcpy(FormatID, MM.MetaMetaID, MM.MetaMetaIDLen);
    memcpy(MetaMetaInfo, MM.MetaMetaInfo, MM.MetaMetaInfoLen);
    load_external_format_FMcontext(FMContext_from_FFS(ReaderFFSContext),
                                   FormatID, MM.MetaMetaIDLen, MetaMetaInfo);
    free(FormatID);
}

BP5Deserializer::ControlInfo *BP5Deserializer::GetPriorControl(FMFormat Format)
{
    struct ControlInfo *tmp = ControlBlocks;
    while (tmp)
    {
        if (tmp->Format == Format)
        {
            return tmp;
        }
        tmp = tmp->Next;
    }
    return NULL;
}

bool BP5Deserializer::NameIndicatesArray(const char *Name)
{
    int Len = strlen(Name);
    return (strcmp("Dims", Name + Len - 4) == 0);
}

DataType BP5Deserializer::TranslateFFSType2ADIOS(const char *Type, int size)
{
    if (strcmp(Type, "integer") == 0)
    {
        if (size == 1)
        {
            return DataType::Int8;
        }
        else if (size == 2)
        {
            return DataType::Int16;
        }
        else if (size == 4)
        {
            return DataType::Int32;
        }
        else if (size == 8)
        {
            return DataType::Int64;
        }
    }
    else if (strcmp(Type, "unsigned integer") == 0)
    {
        if (size == 1)
        {
            return DataType::UInt8;
        }
        else if (size == 2)
        {
            return DataType::UInt16;
        }
        else if (size == 4)
        {
            return DataType::UInt32;
        }
        else if (size == 8)
        {
            return DataType::UInt64;
        }
    }
    else if ((strcmp(Type, "double") == 0) || (strcmp(Type, "float") == 0))
    {
        if (size == sizeof(float))
        {
            return DataType::Float;
        }
        else if ((sizeof(long double) != sizeof(double)) &&
                 (size == sizeof(long double)))
        {
            return DataType::Double;
        }
        else
        {
            return DataType::Double;
        }
    }
    else if (strcmp(Type, "complex4") == 0)
    {
        return DataType::FloatComplex;
    }
    else if (strcmp(Type, "complex8") == 0)
    {
        return DataType::DoubleComplex;
    }
    return DataType::None;
}

void BP5Deserializer::BreakdownVarName(const char *Name, char **base_name_p,
                                       DataType *type_p, int *element_size_p)
{
    int Type;
    int ElementSize;
    const char *NameStart = strchr(strchr(Name, '_') + 1, '_') + 1;
    sscanf(Name, "SST%d_%d_", &ElementSize, &Type);
    *element_size_p = ElementSize;
    *type_p = (DataType)Type;
    *base_name_p = strdup(NameStart);
}

void BP5Deserializer::BreakdownArrayName(const char *Name, char **base_name_p,
                                         DataType *type_p, int *element_size_p)
{
    int Type;
    int ElementSize;
    const char *NameStart = strchr(strchr(Name, '_') + 1, '_') + 1;
    sscanf(Name, "SST%d_%d_", &ElementSize, &Type);
    *element_size_p = ElementSize;
    *type_p = (DataType)Type;
    *base_name_p = strdup(NameStart);
    (*base_name_p)[strlen(*base_name_p) - 4] = 0; // kill "Dims"
}

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByKey(void *Key)
{
    auto ret = VarByKey[Key];
    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByName(const char *Name)
{
    auto ret = VarByName[Name];
    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::CreateVarRec(const char *ArrayName)
{
    BP5VarRec *Ret = new BP5VarRec(m_WriterCohortSize);
    Ret->VarName = strdup(ArrayName);
    Ret->Variable = nullptr;
    VarByName[Ret->VarName] = Ret;
    return Ret;
}

BP5Deserializer::ControlInfo *BP5Deserializer::BuildControl(FMFormat Format)
{
    FMStructDescList FormatList = format_list_of_FMFormat(Format);
    FMFieldList FieldList = FormatList[0].field_list;
    while (strncmp(FieldList->field_name, "BitField", 8) == 0)
        FieldList++;
    while (FieldList->field_name &&
           (strncmp(FieldList->field_name, "DataBlockSize", 8) == 0))
        FieldList++;
    int i = 0;
    int ControlCount = 0;
    ControlInfo *ret = (BP5Deserializer::ControlInfo *)malloc(sizeof(*ret));
    ret->Format = Format;
    while (FieldList[i].field_name)
    {
        ret = (ControlInfo *)realloc(
            ret, sizeof(*ret) + ControlCount * sizeof(struct ControlInfo));
        struct ControlStruct *C = &(ret->Controls[ControlCount]);
        ControlCount++;

        C->FieldIndex = i;
        C->FieldOffset = FieldList[i].field_offset;

        if (NameIndicatesArray(FieldList[i].field_name))
        {
            char *ArrayName;
            DataType Type;
            BP5VarRec *VarRec = nullptr;
            int ElementSize;
            C->IsArray = 1;
            BreakdownArrayName(FieldList[i].field_name, &ArrayName, &Type,
                               &ElementSize);
            //            if (WriterRank != 0)
            //            {
            VarRec = LookupVarByName(ArrayName);
            //            }
            if (!VarRec)
            {
                VarRec = CreateVarRec(ArrayName);
                VarRec->Type = Type;
                VarRec->ElementSize = ElementSize;
                C->ElementSize = ElementSize;
            }
            i += 7; // number of fields in MetaArrayRec
            free(ArrayName);
            C->VarRec = VarRec;
        }
        else
        {
            /* simple field */
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip SST_
            BP5VarRec *VarRec = NULL;
            C->IsArray = 0;
            VarRec = LookupVarByName(FieldName);
            if (!VarRec)
            {
                DataType Type = TranslateFFSType2ADIOS(FieldList[i].field_type,
                                                       FieldList[i].field_size);
                VarRec = CreateVarRec(FieldName);
                VarRec->DimCount = 0;
                C->Type = Type;
                VarRec->Type = Type;
            }
            VarRec->ElementSize = FieldList[i].field_size;
            C->ElementSize = FieldList[i].field_size;
            C->VarRec = VarRec;
            free(FieldName);
            i++;
        }
    }
    ret->ControlCount = ControlCount;
    ret->Next = ControlBlocks;
    ControlBlocks = ret;
    return ret;
}

void BP5Deserializer::ReverseDimensions(size_t *Dimensions, int count)
{
    for (int i = 0; i < count / 2; i++)
    {
        size_t tmp = Dimensions[i];
        Dimensions[i] = Dimensions[count - i - 1];
        Dimensions[count - i - 1] = tmp;
    }
}

void *BP5Deserializer::VarSetup(core::Engine *engine, const char *variableName,
                                const DataType Type, void *data)
{
    if (Type == adios2::DataType::Compound)
    {
        return (void *)NULL;
    }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable =                                          \
            &(engine->m_IO.DefineVariable<T>(variableName));                   \
        variable->SetData((T *)data);                                          \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    return (void *)NULL;
};

void *BP5Deserializer::ArrayVarSetup(core::Engine *engine,
                                     const char *variableName,
                                     const DataType type, int DimCount,
                                     size_t *Shape, size_t *Start,
                                     size_t *Count)
{
    std::vector<size_t> VecShape;
    std::vector<size_t> VecStart;
    std::vector<size_t> VecCount;
    adios2::DataType Type = (adios2::DataType)type;
    /*
     * setup shape of array variable as global (I.E. Count == Shape,
     * Start == 0)
     */
    if (Shape)
    {
        for (int i = 0; i < DimCount; i++)
        {
            VecShape.push_back(Shape[i]);
            VecStart.push_back(0);
            VecCount.push_back(Shape[i]);
        }
    }
    else
    {
        VecShape = {};
        VecStart = {};
        for (int i = 0; i < DimCount; i++)
        {
            VecCount.push_back(Count[i]);
        }
    }

    if (Type == adios2::DataType::Compound)
    {
        return (void *)NULL;
    }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable = &(engine->m_IO.DefineVariable<T>(        \
            variableName, VecShape, VecStart, VecCount));                      \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    return (void *)NULL;
};

void BP5Deserializer::SetupForTimestep(size_t Timestep)
{
    CurTimestep = Timestep;
    PendingRequests.clear();
    for (auto RecPair : VarByKey)
    {
        RecPair.second->Variable = NULL;
    }
}

void BP5Deserializer::InstallMetaData(void *MetadataBlock, size_t BlockLen,
                                      size_t WriterRank, size_t Step)
{
    FFSTypeHandle FFSformat;
    void *BaseData;
    static int DumpMetadata = -1;
    FFSformat =
        FFSTypeHandle_from_encode(ReaderFFSContext, (char *)MetadataBlock);
    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, (char *)MetadataBlock);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(ReaderFFSContext, FFSformat, List);
        FMfree_struct_list(List);
    }
    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(ReaderFFSContext, (char *)MetadataBlock, &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            ReaderFFSContext, (char *)MetadataBlock, BlockLen);
        BaseData = malloc(DecodedLength);
        FFSdecode_to_buffer(ReaderFFSContext, (char *)MetadataBlock, BaseData);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("BP5DumpMetadata") != NULL);
    }
    if (DumpMetadata && (WriterRank == 0))
    {
        printf("\nIncomingMetadatablock from WriterRank %d is %p :\n",
               (int)WriterRank, BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    struct ControlInfo *Control;
    struct ControlStruct *ControlArray;
    Control = GetPriorControl(FMFormat_of_original(FFSformat));
    if (!Control)
    {
        Control = BuildControl(FMFormat_of_original(FFSformat));
    }
    ControlArray = &Control->Controls[0];

    //    if (m_RandomAccessMode) {
    //	PrepareForTimestep(Step);
    //    }
    MetadataBaseAddrs[WriterRank] = BaseData;
    //    } else {
    //	Loaded
    for (int i = 0; i < Control->ControlCount; i++)
    {
        int FieldOffset = ControlArray[i].FieldOffset;
        BP5VarRec *VarRec = ControlArray[i].VarRec;
        void *field_data = (char *)BaseData + FieldOffset;
        if (!FFSBitfieldTest((FFSMetadataInfoStruct *)BaseData, i))
        {
            continue;
        }
        VarRec->PerWriterMetaFieldOffset[WriterRank] = FieldOffset;
        if (ControlArray[i].IsArray)
        {
            MetaArrayRec *meta_base = (MetaArrayRec *)field_data;
            if ((meta_base->Dims > 1) &&
                (m_WriterIsRowMajor != m_ReaderIsRowMajor))
            {
                /* if we're getting data from someone of the other array gender,
                 * switcheroo */
                ReverseDimensions(meta_base->Shape, meta_base->Dims);
                ReverseDimensions(meta_base->Count, meta_base->Dims);
                ReverseDimensions(meta_base->Offsets, meta_base->Dims);
            }
            if (WriterRank == 0)
            {
                // use the shape from rank 0
                VarRec->GlobalDims = meta_base->Shape;
            }
            else if (VarRec->GlobalDims == NULL)
            {
                // unless there wasn't one before
                VarRec->GlobalDims = meta_base->Shape;
            }
            if (!VarRec->Variable)
            {
                VarRec->Variable = ArrayVarSetup(
                    m_Engine, VarRec->VarName, VarRec->Type, meta_base->Dims,
                    meta_base->Shape, meta_base->Offsets, meta_base->Count);
                VarByKey[VarRec->Variable] = VarRec;
            }
            VarRec->DimCount = meta_base->Dims;
            size_t BlockCount =
                meta_base->Dims ? meta_base->DBCount / meta_base->Dims : 1;
            VarRec->PerWriterDataLocation[WriterRank] = meta_base->DataLocation;
            if (WriterRank == 0)
            {
                VarRec->PerWriterBlockStart[WriterRank] = 0;
                if (m_WriterCohortSize > 1)
                    VarRec->PerWriterBlockStart[WriterRank + 1] = BlockCount;
            }
            if (WriterRank < static_cast<size_t>(m_WriterCohortSize - 1))
            {
                VarRec->PerWriterBlockStart[WriterRank + 1] =
                    VarRec->PerWriterBlockStart[WriterRank] + BlockCount;
            }
        }
        else
        {
            if (!VarRec->Variable)
            {
                VarRec->Variable = VarSetup(m_Engine, VarRec->VarName,
                                            VarRec->Type, field_data);
                VarByKey[VarRec->Variable] = VarRec;
            }
        }
    }
}

void BP5Deserializer::InstallAttributeData(void *AttributeBlock,
                                           size_t BlockLen, size_t Step)
{
    static int DumpMetadata = -1;
    FMFieldList FieldList;
    FMStructDescList FormatList;
    void *BaseData;
    FFSTypeHandle FFSformat;

    if (BlockLen == 0)
        return;

    m_Engine->m_IO.RemoveAllAttributes();
    FFSformat =
        FFSTypeHandle_from_encode(ReaderFFSContext, (char *)AttributeBlock);
    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, (char *)AttributeBlock);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(ReaderFFSContext, FFSformat, List);
        FMfree_struct_list(List);
    }

    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(ReaderFFSContext, (char *)AttributeBlock, &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            ReaderFFSContext, (char *)AttributeBlock, BlockLen);
        BaseData = malloc(DecodedLength);
        FFSBuffer decode_buf =
            create_fixed_FFSBuffer((char *)BaseData, DecodedLength);
        FFSdecode_to_buffer(ReaderFFSContext, (char *)AttributeBlock,
                            decode_buf);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("BP5DumpMetadata") != NULL);
    }
    if (DumpMetadata)
    {
        printf("\nIncomingAttributeDatablock is %p :\n", BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    int i = 0;
    while (FieldList[i].field_name)
    {
        char *FieldName;
        void *field_data = (char *)BaseData + FieldList[i].field_offset;

        DataType Type;
        int ElemSize;
        BreakdownVarName(FieldList[i].field_name, &FieldName, &Type, &ElemSize);
        if (Type == adios2::DataType::Compound)
        {
            return;
        }
        else if (Type == helper::GetDataType<std::string>())
        {
            m_Engine->m_IO.DefineAttribute<std::string>(FieldName,
                                                        *(char **)field_data);
        }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        m_Engine->m_IO.DefineAttribute<T>(FieldName, *(T *)field_data);        \
    }

        ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
        else
        {
            std::cout << "Loading attribute matched no type " << ToString(Type)
                      << std::endl;
        }
        free(FieldName);
        i++;
    }
}

bool BP5Deserializer::QueueGet(core::VariableBase &variable, void *DestData)
{
    if (variable.m_SingleValue)
    {
        int WriterRank = 0;
        if (variable.m_SelectionType == adios2::SelectionType::WriteBlock)
            WriterRank = variable.m_BlockID;

        BP5VarRec *VarRec = VarByKey[&variable];
        char *src = ((char *)MetadataBaseAddrs[WriterRank]) +
                    VarRec->PerWriterMetaFieldOffset[WriterRank];
        memcpy(DestData, src, variable.m_ElementSize);
        return false;
    }
    if (variable.m_SelectionType == adios2::SelectionType::BoundingBox)
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarByKey[&variable];
        Req.RequestType = Global;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        Req.Start = variable.m_Start;
        Req.Data = DestData;
        PendingRequests.push_back(Req);
    }
    else if (variable.m_SelectionType == adios2::SelectionType::WriteBlock)
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarByKey[&variable];
        Req.RequestType = Local;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        Req.Data = DestData;
        PendingRequests.push_back(Req);
    }
    else
    {
    }
    return true;
}

bool BP5Deserializer::NeedWriter(BP5ArrayRequest Req, size_t WriterRank)
{
    MetaArrayRec *writer_meta_base =
        (MetaArrayRec *)(((char *)MetadataBaseAddrs[WriterRank]) +
                         Req.VarRec->PerWriterMetaFieldOffset[WriterRank]);

    if (Req.RequestType == Local)
    {
        size_t WriterBlockCount =
            writer_meta_base->Dims
                ? writer_meta_base->DBCount / writer_meta_base->Dims
                : 1;
        size_t NodeFirst = Req.VarRec->PerWriterBlockStart[WriterRank];
        size_t NodeLast = WriterBlockCount + NodeFirst - 1;
        bool res = (NodeFirst <= Req.BlockID) && (NodeLast >= Req.BlockID);
        return res;
    }
    // else Global case
    for (size_t j = 0; j < writer_meta_base->Dims; j++)
    {
        size_t SelOffset = Req.Start[j];
        size_t SelSize = Req.Count[j];
        size_t RankOffset;
        size_t RankSize;

        if (writer_meta_base->Offsets == NULL)
        /* this writer didn't write */
        {
            return false;
        }
        RankOffset = writer_meta_base->Offsets[j];
        RankSize = writer_meta_base->Count[j];
        if ((SelSize == 0) || (RankSize == 0))
        {
            return false;
        }
        if ((RankOffset < SelOffset && (RankOffset + RankSize) <= SelOffset) ||
            (RankOffset >= SelOffset + SelSize))
        {
            return false;
        }
    }
    return true;
}

std::vector<BP5Deserializer::ReadRequest>
BP5Deserializer::GenerateReadRequests()
{
    std::vector<BP5Deserializer::ReadRequest> Ret;
    for (auto &W : WriterInfo)
    {
        W.Status = Empty;
        W.RawBuffer = NULL;
    }

    for (const auto &Req : PendingRequests)
    {
        for (size_t i = 0; i < m_WriterCohortSize; i++)
        {
            if ((WriterInfo[i].Status != Needed) && (NeedWriter(Req, i)))
            {
                WriterInfo[i].Status = Needed;
            }
        }
    }

    for (size_t i = 0; i < m_WriterCohortSize; i++)
    {
        if (WriterInfo[i].Status == Needed)
        {
            ReadRequest RR;
            RR.Timestep = CurTimestep;
            RR.WriterRank = i;
            RR.StartOffset = 0;
            RR.ReadLength =
                ((struct FFSMetadataInfoStruct *)MetadataBaseAddrs[i])
                    ->DataBlockSize;
            RR.DestinationAddr = (char *)malloc(RR.ReadLength);
            RR.Internal = NULL;
            Ret.push_back(RR);
        }
    }
    return Ret;
}

void BP5Deserializer::FinalizeGets(std::vector<ReadRequest> Requests)
{
    for (const auto &Req : PendingRequests)
    {
        //        ImplementGapWarning(Reqs);
        for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize;
             WriterRank++)
        {
            if (NeedWriter(Req, WriterRank))
            {
                /* if needed this writer fill destination with acquired data */
                int ElementSize = Req.VarRec->ElementSize;
                size_t *GlobalDimensions = Req.VarRec->GlobalDims;
                MetaArrayRec *writer_meta_base =
                    (MetaArrayRec
                         *)(((char *)MetadataBaseAddrs[WriterRank]) +
                            Req.VarRec->PerWriterMetaFieldOffset[WriterRank]);
                int DimCount = writer_meta_base->Dims;
                size_t *RankOffset = writer_meta_base->Offsets;
                const size_t *RankSize = writer_meta_base->Count;
                std::vector<size_t> ZeroSel(DimCount);
                std::vector<size_t> ZeroRankOffset(DimCount);
                std::vector<size_t> ZeroGlobalDimensions(DimCount);
                const size_t *SelOffset = NULL;
                const size_t *SelSize = Req.Count.data();
                int ReqIndex = 0;
                while (Requests[ReqIndex].WriterRank !=
                       static_cast<size_t>(WriterRank))
                    ReqIndex++;
                if (Req.VarRec->PerWriterDataLocation[WriterRank] == NULL)
                {
                    // No Data from this writer
                    continue;
                }
                char *IncomingData =
                    (char *)Requests[ReqIndex].DestinationAddr +
                    Req.VarRec->PerWriterDataLocation[WriterRank][0];
                if (Req.Start.size())
                {
                    SelOffset = Req.Start.data();
                }
                if (Req.RequestType == Local)
                {
                    int LocalBlockID =
                        Req.BlockID -
                        Req.VarRec->PerWriterBlockStart[WriterRank];
                    IncomingData =
                        (char *)Requests[ReqIndex].DestinationAddr +
                        Req.VarRec
                            ->PerWriterDataLocation[WriterRank][LocalBlockID];

                    RankOffset = ZeroRankOffset.data();
                    GlobalDimensions = ZeroGlobalDimensions.data();
                    if (SelOffset == NULL)
                    {
                        SelOffset = ZeroSel.data();
                    }
                    for (int i = 0; i < DimCount; i++)
                    {
                        GlobalDimensions[i] = RankSize[i];
                    }
                }
                if (m_ReaderIsRowMajor)
                {
                    ExtractSelectionFromPartialRM(
                        ElementSize, DimCount, GlobalDimensions, RankOffset,
                        RankSize, SelOffset, SelSize, IncomingData,
                        (char *)Req.Data);
                }
                else
                {
                    ExtractSelectionFromPartialCM(
                        ElementSize, DimCount, GlobalDimensions, RankOffset,
                        RankSize, SelOffset, SelSize, IncomingData,
                        (char *)Req.Data);
                }
            }
        }
    }
    for (const auto &Req : Requests)
    {
        free((char *)Req.DestinationAddr);
    }
    PendingRequests.clear();
}

void BP5Deserializer::MapGlobalToLocalIndex(size_t Dims,
                                            const size_t *GlobalIndex,
                                            const size_t *LocalOffsets,
                                            size_t *LocalIndex)
{
    for (size_t i = 0; i < Dims; i++)
    {
        LocalIndex[i] = GlobalIndex[i] - LocalOffsets[i];
    }
}

int BP5Deserializer::FindOffset(size_t Dims, const size_t *Size,
                                const size_t *Index)
{
    int Offset = 0;
    for (size_t i = 0; i < Dims; i++)
    {
        Offset = Index[i] + (Size[i] * Offset);
    }
    return Offset;
}

static int FindOffsetCM(size_t Dims, const size_t *Size, const size_t *Index)
{
    size_t Offset = 0;
    for (int i = static_cast<int>(Dims - 1); i >= 0; i--)
    {
        Offset = Index[i] + (Size[i] * Offset);
    }

    return std::min(static_cast<size_t>(INT_MAX), Offset);
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*
 *  - ElementSize is the byte size of the array elements
 *  - Dims is the number of dimensions in the variable
 *  - GlobalDims is an array, Dims long, giving the size of each dimension
 *  - PartialOffsets is an array, Dims long, giving the starting offsets per
 *    dimension of this data block in the global array
 *  - PartialCounts is an array, Dims long, giving the size per dimension
 *    of this data block in the global array
 *  - SelectionOffsets is an array, Dims long, giving the starting offsets in
 * the
 *    global array of the output selection.
 *  - SelectionCounts is an array, Dims long, giving the size per dimension
 *    of the output selection.
 *  - InData is the input, a slab of the global array
 *  - OutData is the output, to be filled with the selection array.
 */

/*
 * *******************************
 *
 * ExtractSelectionFromPartial*M both need to be extended to work when
 * the reader and writer have different byte orders.  This involves at
 * least supporting simple big/little-endian byte reversal, but a true
 * archival format should also consider mixed and middle-endian
 * hybrids.  This would require changes to the BP5 header so that the
 * appropriate transformations could be determined.
 *
 * *******************************
 */

// Row major version
void BP5Deserializer::ExtractSelectionFromPartialRM(
    int ElementSize, size_t Dims, const size_t *GlobalDims,
    const size_t *PartialOffsets, const size_t *PartialCounts,
    const size_t *SelectionOffsets, const size_t *SelectionCounts,
    const char *InData, char *OutData)
{
    size_t BlockSize;
    size_t SourceBlockStride = 0;
    size_t DestBlockStride = 0;
    size_t SourceBlockStartOffset;
    size_t DestBlockStartOffset;
    size_t BlockCount;
    size_t OperantDims;
    size_t OperantElementSize;

    BlockSize = 1;
    OperantDims = Dims;
    OperantElementSize = static_cast<size_t>(ElementSize);
    for (int Dim = static_cast<int>(Dims - 1); Dim >= 0; Dim--)
    {
        if ((GlobalDims[Dim] == PartialCounts[Dim]) &&
            (SelectionCounts[Dim] == PartialCounts[Dim]))
        {
            BlockSize *= GlobalDims[Dim];
            OperantDims--; /* last dimension doesn't matter, we got all and we
                               want all */
            OperantElementSize *= GlobalDims[Dim];
        }
        else
        {
            size_t Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
            size_t Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                               SelectionOffsets[Dim] + SelectionCounts[Dim]);
            BlockSize *= (Right - Left);
            break;
        }
    }
    if (OperantDims > 0)
    {
        SourceBlockStride = PartialCounts[OperantDims - 1] * OperantElementSize;
        DestBlockStride = SelectionCounts[OperantDims - 1] * OperantElementSize;
    }

    /* calculate first selected element and count */
    BlockCount = 1;
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
    for (size_t Dim = 0; Dim < Dims; Dim++)
    {
        size_t Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        size_t Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                           SelectionOffsets[Dim] + SelectionCounts[Dim]);
        if ((OperantDims != 0) && (Dim < OperantDims - 1))
        {
            BlockCount *= (Right - Left);
        }
        FirstIndex[Dim] = Left;
    }
    size_t *SelectionIndex = (size_t *)malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffset(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= ElementSize;

    size_t *PartialIndex = (size_t *)malloc(Dims * sizeof(PartialIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, PartialOffsets, PartialIndex);
    SourceBlockStartOffset = FindOffset(Dims, PartialCounts, PartialIndex);
    free(PartialIndex);
    SourceBlockStartOffset *= ElementSize;

    InData += SourceBlockStartOffset;
    OutData += DestBlockStartOffset;
    size_t i;
    for (i = 0; i < BlockCount; i++)
    {
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
}

// Column-major version
void BP5Deserializer::ExtractSelectionFromPartialCM(
    int ElementSize, size_t Dims, const size_t *GlobalDims,
    const size_t *PartialOffsets, const size_t *PartialCounts,
    const size_t *SelectionOffsets, const size_t *SelectionCounts,
    const char *InData, char *OutData)
{
    int BlockSize;
    int SourceBlockStride = 0;
    int DestBlockStride = 0;
    int SourceBlockStartOffset;
    int DestBlockStartOffset;
    int BlockCount;
    int OperantElementSize;

    BlockSize = 1;
    OperantElementSize = ElementSize;
    for (size_t Dim = 0; Dim < Dims; Dim++)
    {
        if ((GlobalDims[Dim] == PartialCounts[Dim]) &&
            (SelectionCounts[Dim] == PartialCounts[Dim]))
        {
            BlockSize *= GlobalDims[Dim];
            OperantElementSize *= GlobalDims[Dim];
            /* skip the first bit of everything */
            GlobalDims++;
            PartialOffsets++;
            PartialCounts++;
            SelectionOffsets++;
            SelectionCounts++;
            Dims--;
            /* and make sure we do the next dimensions appropriately by
             * repeating this iterator value */
            Dim--;
        }
        else
        {
            int Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
            int Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                            SelectionOffsets[Dim] + SelectionCounts[Dim]);
            BlockSize *= (Right - Left);
            break;
        }
    }
    if (Dims > 0)
    {
        SourceBlockStride = PartialCounts[0] * OperantElementSize;
        DestBlockStride = SelectionCounts[0] * OperantElementSize;
    }

    /* calculate first selected element and count */
    BlockCount = 1;
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
    for (size_t Dim = 0; Dim < Dims; Dim++)
    {
        int Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        int Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                        SelectionOffsets[Dim] + SelectionCounts[Dim]);
        if (Dim > 0)
        {
            BlockCount *= (Right - Left);
        }
        FirstIndex[Dim] = Left;
    }
    size_t *SelectionIndex = (size_t *)malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffsetCM(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= OperantElementSize;

    size_t *PartialIndex = (size_t *)malloc(Dims * sizeof(PartialIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, PartialOffsets, PartialIndex);
    SourceBlockStartOffset = FindOffsetCM(Dims, PartialCounts, PartialIndex);

    free(PartialIndex);
    SourceBlockStartOffset *= OperantElementSize;

    InData += SourceBlockStartOffset;
    OutData += DestBlockStartOffset;
    for (int i = 0; i < BlockCount; i++)
    {
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
}

BP5Deserializer::BP5Deserializer(int WriterCount, bool WriterIsRowMajor,
                                 bool ReaderIsRowMajor)
: m_WriterIsRowMajor{WriterIsRowMajor}, m_ReaderIsRowMajor{ReaderIsRowMajor},
  m_WriterCohortSize{static_cast<size_t>(WriterCount)}
{
    FMContext Tmp = create_local_FMcontext();
    ReaderFFSContext = create_FFSContext_FM(Tmp);
    free_FMcontext(Tmp);
    WriterInfo.resize(m_WriterCohortSize);
    MetadataBaseAddrs.resize(m_WriterCohortSize);
}

BP5Deserializer::~BP5Deserializer()
{
    free_FFSContext(ReaderFFSContext);
    for (size_t i = 0; i < m_WriterCohortSize; i++)
    {
        if (WriterInfo[i].RawBuffer)
            free(WriterInfo[i].RawBuffer);
    }
    struct ControlInfo *tmp = ControlBlocks;
    ControlBlocks = NULL;
    while (tmp)
    {
        struct ControlInfo *next = tmp->Next;
        free(tmp);
        tmp = next;
    }
    for (auto &VarRec : VarByName)
    {
        free(VarRec.second->VarName);
        delete VarRec.second;
    }
}

Engine::MinVarInfo *BP5Deserializer::MinBlocksInfo(const VariableBase &Var,
                                                   const size_t Step)
{
    BP5VarRec *VarRec = LookupVarByKey((void *)&Var);
    MetaArrayRec *meta_base =
        (MetaArrayRec *)(((char *)MetadataBaseAddrs[0]) +
                         VarRec->PerWriterMetaFieldOffset[0]);
    Engine::MinVarInfo *MV =
        new Engine::MinVarInfo(meta_base->Dims, meta_base->Shape);

    MV->Dims = meta_base->Dims;
    MV->Shape = meta_base->Shape;
    MV->IsReverseDims =
        ((meta_base->Dims > 1) && (m_WriterIsRowMajor != m_ReaderIsRowMajor));

    int Id = 0;
    for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize; WriterRank++)
    {
        MetaArrayRec *writer_meta_base =
            (MetaArrayRec *)(((char *)MetadataBaseAddrs[WriterRank]) +
                             VarRec->PerWriterMetaFieldOffset[WriterRank]);
        size_t WriterBlockCount =
            writer_meta_base->Dims
                ? writer_meta_base->DBCount / writer_meta_base->Dims
                : 1;
        Id += WriterBlockCount;
    }
    MV->BlocksInfo.reserve(Id);

    Id = 0;
    for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize; WriterRank++)
    {
        MetaArrayRec *writer_meta_base =
            (MetaArrayRec *)(((char *)MetadataBaseAddrs[WriterRank]) +
                             VarRec->PerWriterMetaFieldOffset[WriterRank]);

        size_t WriterBlockCount =
            meta_base->Dims ? meta_base->DBCount / meta_base->Dims : 1;
        for (size_t i = 0; i < WriterBlockCount; i++)
        {
            size_t *Offsets = NULL;
            size_t *Count = NULL;
            if (writer_meta_base->Offsets)
                Offsets = writer_meta_base->Offsets + (i * meta_base->Dims);
            if (writer_meta_base->Count)
                Count = writer_meta_base->Count + (i * meta_base->Dims);
            Engine::MinBlockInfo Blk;
            Blk.WriterID = WriterRank;
            Blk.BlockID = Id++;
            Blk.Start = Offsets;
            Blk.Count = Count;
            // Blk.MinUnion
            // Blk.MaxUnion
            // Blk.BufferP
            MV->BlocksInfo.push_back(Blk);
        }
    }
    return MV;
}

}
}
