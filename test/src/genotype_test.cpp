#ifndef GENOTYPE_TEST_HPP
#define GENOTYPE_TEST_HPP
#include "genotype.hpp"
#include "region.hpp"
#include "gtest/gtest.h"
class GENOTYPE_BASIC : public Genotype, public ::testing::Test
{
public:
    void cleanup()
    {
        m_keep_file = "";
        m_sample_file = "";
        m_remove_file = "";
        m_genotype_file_names.clear();
        m_delim = "";
        m_ignore_fid = false;
    }

    void sample_generation_check(
        const std::unordered_set<std::string>& founder_info,
        const bool in_regress, const bool cal_prs, const bool cal_ld,
        const bool keep, const bool duplicated,
        const std::vector<std::string>& input,
        std::unordered_set<std::string>& processed_sample,
        std::vector<Sample_ID>& result,
        std::vector<std::string>& duplicated_sample, size_t& cur_idx,
        bool remove_sample = true, bool ignore_fid = false)
    {
        m_ignore_fid = ignore_fid;
        m_remove_sample = remove_sample;
        const size_t before = duplicated_sample.size();
        std::vector<std::string> token = input;
        gen_sample(+FAM::FID, +FAM::IID, +FAM::SEX, +FAM::FATHER, +FAM::MOTHER,
                   cur_idx, founder_info, token[+FAM::PHENOTYPE], token, result,
                   processed_sample, duplicated_sample);
        if (!keep)
        {
            ASSERT_EQ(result.size(), cur_idx);
            // shouldn't update the duplicated sample list
            ASSERT_EQ(before, duplicated_sample.size());
        }
        else if (duplicated)
        {
            ASSERT_EQ(duplicated_sample.size(), before + 1);
        }
        else
        {
            ASSERT_EQ(result.size(), cur_idx + 1);
            if (!m_ignore_fid)
            {
                ASSERT_NE(processed_sample.find(token[+FAM::FID] + m_delim
                                                + token[+FAM::IID]),
                          processed_sample.end());
            }
            else
            {
                ASSERT_NE(processed_sample.find(token[+FAM::IID]),
                          processed_sample.end());
            }
            ASSERT_STREQ(result.back().pheno.c_str(),
                         token[+FAM::PHENOTYPE].c_str());
            ASSERT_EQ(result.back().in_regression, in_regress);
            if (!m_ignore_fid)
                ASSERT_STREQ(result.back().FID.c_str(),
                             token[+FAM::FID].c_str());
            ASSERT_STREQ(result.back().IID.c_str(), token[+FAM::IID].c_str());
            ASSERT_EQ(IS_SET(m_calculate_prs.data(), cur_idx), cal_prs);
            ASSERT_EQ(IS_SET(m_sample_for_ld.data(), cur_idx), cal_ld);
            ASSERT_EQ(before, duplicated_sample.size());
            ++cur_idx;
        }
    }
};
// This should be the simplest of the three genotype class. Test anything that
// doesn't require binaryplink and binarygen
// set_genotype_files

TEST_F(GENOTYPE_BASIC, CHR_CONVERT)
{
    ASSERT_FALSE(chr_prefix("1"));
    ASSERT_TRUE(chr_prefix("chr1"));
    auto chr = get_chrom_code("1");
    ASSERT_EQ(chr, 1);
    chr = get_chrom_code("chr10");
    ASSERT_EQ(chr, 10);
    chr = get_chrom_code("CHRX");
    ASSERT_EQ(chr, CHROM_X);
    chr = get_chrom_code("M");
    ASSERT_EQ(chr, CHROM_MT);
    chr = get_chrom_code("Y");
    ASSERT_EQ(chr, CHROM_Y);
    chr = get_chrom_code("XY");
    ASSERT_EQ(chr, CHROM_XY);
    chr = get_chrom_code("mt");
    ASSERT_EQ(chr, CHROM_MT);
    chr = get_chrom_code("CHR Happy");
    ASSERT_EQ(chr, -1);
    chr = get_chrom_code("chromosome");
    ASSERT_EQ(chr, -1);
    chr = get_chrom_code("0X");
    ASSERT_EQ(chr, CHROM_X);
    chr = get_chrom_code("0Y");
    ASSERT_EQ(chr, CHROM_Y);
    chr = get_chrom_code("0M");
    ASSERT_EQ(chr, CHROM_MT);
    // TODO: Add more crazy chromosome use cases
}

TEST_F(GENOTYPE_BASIC, INITIALIZE)
{
    GenoFile geno;
    geno.num_autosome = 22;
    Phenotype pheno;
    pheno.ignore_fid = false;
    std::string delim = " ";
    std::string type = "bed";
    Reporter reporter(std::string("LOG"), 60, true);
    // need to check the following
    // m_genotype_file_name is set
    // m_sample_file is set when external file is provided and empty if not
    // m_keep_file, m_remove_file and m_delim should be set
    // there should be no error check
    // Normal input
    geno.file_name = "Genotype";
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_STREQ(delim.c_str(), m_delim.c_str());
    ASSERT_EQ(m_genotype_file_names.size(), 1);
    ASSERT_STREQ(geno.file_name.c_str(), m_genotype_file_names.front().c_str());
    ASSERT_TRUE(m_keep_file.empty());
    ASSERT_TRUE(m_remove_file.empty());
    ASSERT_TRUE(m_sample_file.empty());
    ASSERT_FALSE(m_ignore_fid);
    cleanup();
    // now with external sample_file
    geno.file_name = "Genotype,External";
    delim = "\t";
    pheno.ignore_fid = true;
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_STREQ(delim.c_str(), m_delim.c_str());
    ASSERT_EQ(m_genotype_file_names.size(), 1);
    ASSERT_STREQ("Genotype", m_genotype_file_names.front().c_str());
    ASSERT_TRUE(m_keep_file.empty());
    ASSERT_TRUE(m_remove_file.empty());
    ASSERT_TRUE(m_ignore_fid);
    ASSERT_STREQ(m_sample_file.c_str(), "External");
    // now check replacement
    cleanup();
    geno.file_name = "chr#_geno";
    geno.keep = "Hi";
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_EQ(m_genotype_file_names.size(), geno.num_autosome);
    for (size_t i = 0; i < geno.num_autosome; ++i)
    {
        std::string cur_file = "chr" + std::to_string(i + 1) + "_geno";
        ASSERT_STREQ(m_genotype_file_names[i].c_str(), cur_file.c_str());
    }
    ASSERT_STREQ(m_keep_file.c_str(), "Hi");
    ASSERT_TRUE(m_remove_file.empty());
    ASSERT_TRUE(m_sample_file.empty());
    cleanup();
    // invalid format
    geno.file_name = "chr,x,y";
    try
    {
        initialize(geno, pheno, delim, type, &reporter);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    cleanup();
    geno.file_name = "chr#_geno,Sample";
    geno.keep = "Hi";
    geno.num_autosome = 30;
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_EQ(m_genotype_file_names.size(), geno.num_autosome);
    for (size_t i = 0; i < geno.num_autosome; ++i)
    {
        std::string cur_file = "chr" + std::to_string(i + 1) + "_geno";
        ASSERT_STREQ(m_genotype_file_names[i].c_str(), cur_file.c_str());
    }
    ASSERT_STREQ(m_keep_file.c_str(), "Hi");
    ASSERT_TRUE(m_remove_file.empty());
    ASSERT_STREQ(m_sample_file.c_str(), "Sample");
    // finally, with list
    cleanup();
    // we need a dummy file
    std::ofstream dummy("DUMMY");
    std::vector<std::string> expected = {"A", "B", "C D", "E F G", "H#I"};
    for (auto& d : expected) { dummy << d << std::endl; }
    dummy.close();
    geno.file_list = "DUMMY";
    geno.file_name = "";
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_EQ(m_genotype_file_names.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    { ASSERT_STREQ(m_genotype_file_names[i].c_str(), expected[i].c_str()); }
    ASSERT_TRUE(m_sample_file.empty());
    // now try with external sample
    cleanup();
    geno.file_list = "DUMMY,sample";
    initialize(geno, pheno, delim, type, &reporter);
    ASSERT_EQ(m_genotype_file_names.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
    { ASSERT_STREQ(m_genotype_file_names[i].c_str(), expected[i].c_str()); }
    ASSERT_FALSE(m_sample_file.empty());
    ASSERT_STREQ(m_sample_file.c_str(), "sample");

    // invalid file input
    cleanup();
    geno.file_list = "DUMMY,B,C";
    try
    {
        initialize(geno, pheno, delim, type, &reporter);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    cleanup();
    std::remove("DUMMY");
    cleanup();
    // should fail here when the file is not found
    try
    {
        initialize(geno, pheno, delim, type, &reporter);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
}

TEST_F(GENOTYPE_BASIC, READ_BASE_PARSE_CHR)
{
    init_chr();
    // we have already check the chr convertion work
    // so we just need to check if the return is correct
    std::string normal = "Chr6 1023 rs1234 A T";
    // wrong code
    std::string wrong = "chromosome 1023 rs1234 A T";
    // too large
    std::string large = "Chr30 1023 rs1234 A T";
    // sex
    std::string sex = "chrX 1234 rs54321 G C";
    std::vector<std::string_view> token = misc::tokenize(normal);
    // if we don't have chr, we return ~size_t(0);
    BaseFile base_file;
    base_file.has_column[+BASE_INDEX::CHR] = false;
    size_t chr;
    ASSERT_EQ(parse_chr(token, base_file, +BASE_INDEX::CHR, chr), 0);
    ASSERT_EQ(chr, ~size_t(0));
    // now we have chr info
    base_file.has_column[+BASE_INDEX::CHR] = true;
    base_file.column_index[+BASE_INDEX::CHR] = 0;
    ASSERT_EQ(parse_chr(token, base_file, +BASE_INDEX::CHR, chr), 0);
    ASSERT_EQ(chr, 6);
    // check wrong code
    token = misc::tokenize(wrong);
    ASSERT_EQ(parse_chr(token, base_file, +BASE_INDEX::CHR, chr), 1);
    // check large code

    token = misc::tokenize(large);
    ASSERT_EQ(parse_chr(token, base_file, +BASE_INDEX::CHR, chr), 2);
    // sex code
    token = misc::tokenize(sex);
    ASSERT_EQ(parse_chr(token, base_file, +BASE_INDEX::CHR, chr), 2);
}

TEST_F(GENOTYPE_BASIC, READ_BASE_ALLELE)
{
    std::string line = "Chr1 1023 rs1234 a c";
    BaseFile base_file;
    std::vector<std::string_view> token = misc::tokenize(line);
    std::string ref_allele;
    parse_allele(token, base_file, +BASE_INDEX::EFFECT, ref_allele);
    ASSERT_TRUE(ref_allele.empty());
    base_file.column_index[+BASE_INDEX::EFFECT] = 3;
    base_file.has_column[+BASE_INDEX::EFFECT] = true;
    parse_allele(token, base_file, +BASE_INDEX::EFFECT, ref_allele);
    ASSERT_STREQ(ref_allele.c_str(), "A");
}

TEST_F(GENOTYPE_BASIC, READ_BASE_LOC)
{
    std::string line = "chr1 1234 rs1234 a c";
    // ok if not provided
    BaseFile base_file;
    std::vector<std::string_view> token = misc::tokenize(line);
    size_t loc;
    ASSERT_TRUE(parse_loc(token, base_file, +BASE_INDEX::BP, loc));
    ASSERT_EQ(loc, ~size_t(0));
    // we are ok as long as it is valid number
    base_file.has_column[+BASE_INDEX::BP] = true;
    base_file.column_index[+BASE_INDEX::BP] = 1;
    ASSERT_TRUE(parse_loc(token, base_file, +BASE_INDEX::BP, loc));
    ASSERT_EQ(loc, 1234);
    // we have already tested the conversion, just need 1 fail case
    line = "chr12 -123 rs12345 a c";
    token = misc::tokenize(line);
    ASSERT_FALSE(parse_loc(token, base_file, +BASE_INDEX::BP, loc));
}

TEST_F(GENOTYPE_BASIC, READ_BASE_FILTER_BY_VALUE)
{
    std::string line = "chr1 1234 rs1234 a c 0.1 0.3 0.9";
    std::vector<std::string_view> token = misc::tokenize(line);
    BaseFile base_file;
    // we will not do filtering when it is not provided
    ASSERT_FALSE(
        base_filter_by_value(token, base_file, 0.05, +BASE_INDEX::MAF));
    base_file.has_column[+BASE_INDEX::MAF] = true;
    base_file.column_index[+BASE_INDEX::MAF] = 6;
    // we will not filter this as 0.05 < 0.3
    ASSERT_FALSE(
        base_filter_by_value(token, base_file, 0.05, +BASE_INDEX::MAF));
    ASSERT_TRUE(base_filter_by_value(token, base_file, 0.4, +BASE_INDEX::MAF));
    // don't filter if it is exact
    ASSERT_FALSE(base_filter_by_value(token, base_file, 0.3, +BASE_INDEX::MAF));
    // and filter if it is not convertable
    line = "chr1 1234 rs1234 a c 0.1 NA 0.9";
    token = misc::tokenize(line);
    ASSERT_TRUE(base_filter_by_value(token, base_file, 0.4, +BASE_INDEX::MAF));
}
TEST_F(GENOTYPE_BASIC, READ_BASE_P)
{
    double pvalue;
    // 0 valid 1 not convert 3 out bound 2 too big
    ASSERT_EQ(parse_pvalue("0.05", 0.67, pvalue), 0);
    ASSERT_DOUBLE_EQ(pvalue, 0.05);
    ASSERT_EQ(parse_pvalue("NA", 0.67, pvalue), 1);
    ASSERT_EQ(parse_pvalue("0.7", 0.67, pvalue), 2);
    ASSERT_EQ(parse_pvalue("2", 0.67, pvalue), 3);
    ASSERT_EQ(parse_pvalue("-1", 0.67, pvalue), 3);
}

TEST_F(GENOTYPE_BASIC, READ_BASE_STAT)
{
    double stat;
    bool OR = true;
    // 0 valid, 1 OR 0 or cannot convert, 2 negative OR
    ASSERT_EQ(parse_stat("0.1236", !OR, stat), 0);
    ASSERT_DOUBLE_EQ(0.1236, stat);
    ASSERT_EQ(parse_stat("0.1236", OR, stat), 0);
    ASSERT_DOUBLE_EQ(log(0.1236), stat);
    ASSERT_EQ(parse_stat("NA", !OR, stat), 1);
    ASSERT_EQ(parse_stat("0", OR, stat), 1);
    ASSERT_EQ(parse_stat("NA", OR, stat), 1);
    ASSERT_EQ(parse_stat("-0.654", !OR, stat), 0);
    ASSERT_DOUBLE_EQ(-0.654, stat);
    ASSERT_EQ(parse_stat("-0.654", OR, stat), 2);
}
TEST_F(GENOTYPE_BASIC, READ_BASE_RS)
{
    std::string line = "Chr1 1023 rs1234 A T";
    std::string line2 = "chr2 1234 rs5432 G T";
    std::vector<std::string_view> token = misc::tokenize(line);
    std::vector<std::string_view> token2 = misc::tokenize(line2);
    BaseFile base_file;
    base_file.has_column[+BASE_INDEX::RS] = false;
    std::unordered_set<std::string> dup_index;
    std::string rs_id;
    // first, check when rs is not provided
    try
    {
        parse_rs_id(token, dup_index, base_file, rs_id);
        FAIL();
    }
    catch (const std::runtime_error&)
    {
        SUCCEED();
    }
    // now we have the correct rs
    base_file.has_column[+BASE_INDEX::RS] = true;
    base_file.column_index[+BASE_INDEX::RS] = 2;
    ASSERT_EQ(parse_rs_id(token, dup_index, base_file, rs_id), 0);
    dup_index.insert(rs_id);
    ASSERT_STREQ(rs_id.c_str(), "rs1234");
    // duplicated SNP
    rs_id.clear();
    ASSERT_EQ(parse_rs_id(token, dup_index, base_file, rs_id), 1);
    // excluding SNP
    m_exclude_snp = true;
    dup_index.clear();
    m_snp_selection_list.insert("rs1234");
    rs_id.clear();
    ASSERT_EQ(parse_rs_id(token, dup_index, base_file, rs_id), 2);
    rs_id.clear();
    ASSERT_EQ(parse_rs_id(token2, dup_index, base_file, rs_id), 0);
    ASSERT_STREQ(rs_id.c_str(), "rs5432");
    // selecting SNP
    rs_id.clear();
    dup_index.clear();
    m_exclude_snp = false;
    ASSERT_EQ(parse_rs_id(token, dup_index, base_file, rs_id), 0);
    ASSERT_STREQ(rs_id.c_str(), "rs1234");
    rs_id.clear();
    ASSERT_EQ(parse_rs_id(token2, dup_index, base_file, rs_id), 2);
}

TEST_F(GENOTYPE_BASIC, SET_FILE_NAME_WITHOUT_HASH)
{
    std::string name = "Test";
    m_genotype_file_names = set_genotype_files(name);
    ASSERT_STREQ(m_genotype_file_names.front().c_str(), name.c_str());
    ASSERT_EQ(m_genotype_file_names.size(), 1);
}

TEST_F(GENOTYPE_BASIC, SET_FILE_NAME_WITH_HASH)
{
    std::string name = "chr#test";
    m_autosome_ct = 22;
    m_genotype_file_names = set_genotype_files(name);
    ASSERT_EQ(m_genotype_file_names.size(), m_autosome_ct);
    for (size_t i = 1; i <= m_autosome_ct; ++i)
    {
        std::string name = "chr" + std::to_string(i) + "test";
        ASSERT_STREQ(m_genotype_file_names[i - 1].c_str(), name.c_str());
    }
}

TEST_F(GENOTYPE_BASIC, SET_FILE_NAME_MULTI_HASH)
{
    // don't think this is a useful case but in theory all # should be
    // substituted
    std::string name = "chr#test#";
    m_autosome_ct = 22;
    m_genotype_file_names = set_genotype_files(name);
    ASSERT_EQ(m_genotype_file_names.size(), m_autosome_ct);
    for (size_t i = 1; i <= m_autosome_ct; ++i)
    {
        std::string name =
            "chr" + std::to_string(i) + "test" + std::to_string(i);
        ASSERT_STREQ(m_genotype_file_names[i - 1].c_str(), name.c_str());
    }
}


TEST_F(GENOTYPE_BASIC, GET_RS_COLUMN)
{
    Reporter reporter(std::string("LOG"), 60, true);
    m_reporter = &reporter;
    // simplest format
    std::string input = "rs1234";
    auto result = get_rs_column(input);
    ASSERT_EQ(result, 0);
    // might want to test the rest, RS.ID RS_ID, RSID, SNP.ID, SNP_ID,
    // Variant_ID, Variant.ID
    input = "A B CHR snp Weight P A1";
    // read from header
    result = get_rs_column(input);
    ASSERT_EQ(result, 3);
    input = "1 rs1234 123 0 A T";
    // this should be bim file
    result = get_rs_column(input);
    ASSERT_EQ(result, 1);
    input = "rs1234 A BC T A D E T";
    // no idea what this is, will take first column
    result = get_rs_column(input);
    ASSERT_EQ(result, 0);
}
TEST_F(GENOTYPE_BASIC, LOAD_SNP_LIST)
{
    Reporter reporter("LOG", 60, true);
    m_reporter = &reporter;
    std::vector<std::string> expected = {"rs1234", "rs76591", "rs139486"};
    std::ofstream dummy("DUMMY");
    for (auto& e : expected) { dummy << e << std::endl; }
    auto res = load_snp_list("DUMMY");
    ASSERT_EQ(res.size(), expected.size());
    for (auto& e : expected) { ASSERT_FALSE(res.find(e) == res.end()); }
    dummy.close();
    std::remove("DUMMY");
    // now different format, just in case the idx is correct (use bim)
    dummy.open("DUMMY");
    for (auto& e : expected) { dummy << "1 " << e << " 3 4 5 6" << std::endl; }
    dummy.close();
    res = load_snp_list("DUMMY");

    ASSERT_EQ(res.size(), expected.size());
    for (auto& e : expected) { ASSERT_FALSE(res.find(e) == res.end()); }
    std::remove("DUMMY");
    // should fail if file not found
    try
    {
        res = load_snp_list("DUMMY");
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
}

TEST_F(GENOTYPE_BASIC, READ_BASE_FULL)
{
    Reporter reporter(std::string("LOG"), 60, true);
    init_chr();
    m_reporter = &reporter;
    m_snp_selection_list.insert("exclude");
    // wrong loc need to test separately as that is a throw condition
    const double maf_case = 0.04, maf_control = 0.05, info = 0.8,
                 max_thres = 0.5;
    std::vector<std::string> base = {
        "CHR BP RS A1 A2 P STAT MAF INFO MAF_CASE",
        "chr1 1234 exclude A C 0.05 1.96 0.1 0.9 0.05",
        "chr1 1234 normal A C 0.05 1.96 0.1 0.9 0.05",
        "chr1 1234 dup A C 0.05 1.96 0.1 0.9 0.05",
        "chr1 1234 dup A C 0.05 1.96 0.1 0.9 0.05",
        "chrX 1234 sex_chr A C 0.07 1.98 0.1 0.9 0.05",
        "chromosome 1234 wrong_chr A C 0.07 1.98 0.1 0.9 0.05",
        "chr1 1234 filter_control A C 0.05 1.96 0.01 0.9 0.05",
        "chr1 1234 filter_case A C 0.05 1.96 0.1 0.9 0.03",
        "chr1 1234 filter_info A C 0.05 1.96 0.1 0.02 0.05",
        "chr1 1234 p_not_convert A C NA 1.96 0.1 0.9 0.05",
        "chr1 1234 p_exclude A C 0.6 1.96 0.1 0.9 0.05",
        "chr1 1234 stat_not_convert A C 0.05 NA 0.1 0.9 0.05",
        "chr1 1234 negative_stat A C 0.05 -1.96 0.1 0.9 0.05",
        "chr6 1234 region_exclude A C 0.05 -1.96 0.1 0.9 0.05",
        "chr1 1234 ambiguous A T 0.05 1.96 0.1 0.9 0.05"};
    std::vector<size_t> expected(+FILTER_COUNT::MAX, 1);
    // skip header, as we are not using is_index
    expected[+FILTER_COUNT::NUM_LINE] = base.size() - 1;
    // both P and stat shares the same count
    expected[+FILTER_COUNT::NOT_CONVERT] = 2;
    // same for maf case and maf control
    expected[+FILTER_COUNT::MAF] = 2;
    std::ofstream dummy("DUMMY");
    for (auto e : base) { dummy << e << std::endl; }
    dummy.close();
    BaseFile base_file;
    base_file.file_name = "DUMMY";
    base_file.has_column[+BASE_INDEX::CHR] = true;
    base_file.has_column[+BASE_INDEX::BP] = true;
    base_file.has_column[+BASE_INDEX::EFFECT] = true;
    base_file.has_column[+BASE_INDEX::INFO] = true;
    base_file.has_column[+BASE_INDEX::MAF] = true;
    base_file.has_column[+BASE_INDEX::MAF_CASE] = true;
    base_file.has_column[+BASE_INDEX::NONEFFECT] = true;
    base_file.has_column[+BASE_INDEX::P] = true;
    base_file.has_column[+BASE_INDEX::RS] = true;
    base_file.has_column[+BASE_INDEX::STAT] = true;
    base_file.column_index[+BASE_INDEX::CHR] = 0;
    base_file.column_index[+BASE_INDEX::BP] = 1;
    base_file.column_index[+BASE_INDEX::RS] = 2;
    base_file.column_index[+BASE_INDEX::EFFECT] = 3;
    base_file.column_index[+BASE_INDEX::NONEFFECT] = 4;
    base_file.column_index[+BASE_INDEX::P] = 5;
    base_file.column_index[+BASE_INDEX::STAT] = 6;
    base_file.column_index[+BASE_INDEX::MAF] = 7;
    base_file.column_index[+BASE_INDEX::INFO] = 8;
    base_file.column_index[+BASE_INDEX::MAF_CASE] = 9;
    base_file.column_index[+BASE_INDEX::MAX] = 9;
    base_file.is_or = true;
    QCFiltering base_qc;
    base_qc.info_score = info;
    base_qc.maf = maf_control;
    base_qc.maf_case = maf_case;
    PThresholding threshold_info;
    threshold_info.no_full = true;
    threshold_info.fastscore = true;
    threshold_info.bar_levels = {max_thres};
    std::vector<IITree<size_t, size_t>> exclusion_regions;
    Region::generate_exclusion(exclusion_regions, "chr6:1-2000");
    auto [filter_count, dup_idx] =
        read_base(base_file, base_qc, threshold_info, exclusion_regions);
    std::remove("DUMMY");
    // 2 because we have both normal and dup in it
    ASSERT_EQ(m_existed_snps.size(), 2);
    // normal must be here
    ASSERT_TRUE(m_existed_snps_index.find("normal")
                != m_existed_snps_index.end());
    ASSERT_EQ(dup_idx.size(), 1);
    ASSERT_TRUE(dup_idx.find("dup") != dup_idx.end());
    ASSERT_EQ(expected.size(), filter_count.size());
    for (size_t i = 0; i < expected.size(); ++i)
    { ASSERT_EQ(filter_count[i], expected[i]); }
    dummy.open("DUMMY");
    dummy << "CHR BP RS A1 A2 P STAT MAF INFO MAF_CASE" << std::endl;
    dummy << "chr1 -1234 negative_loc A C 0.05 1.96 0.1 0.9 0.05" << std::endl;
    dummy.close();
    // should fail due to invalid coordinates
    try
    {
        read_base(base_file, base_qc, threshold_info, exclusion_regions);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    // invalid p-value
    dummy.open("DUMMY");
    dummy << "CHR BP RS A1 A2 P STAT MAF INFO MAF_CASE" << std::endl;
    dummy << "chr1 1234 invalid_p A C -0.05 1.96 0.1 0.9 0.05" << std::endl;
    dummy.close();
    // should fail due to invalid coordinates
    try
    {
        read_base(base_file, base_qc, threshold_info, exclusion_regions);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    std::remove("LOG");
    std::remove("DUMMY");
}
// init_chr
// chr_code_check
// load_snp_list
// load_ref

TEST_F(GENOTYPE_BASIC, AMBIGUOUS)
{
    ASSERT_TRUE(ambiguous("A", "A"));
    ASSERT_TRUE(ambiguous("G", "G"));
    ASSERT_TRUE(ambiguous("C", "C"));
    ASSERT_TRUE(ambiguous("T", "T"));
    ASSERT_TRUE(ambiguous("A", "T"));
    ASSERT_TRUE(ambiguous("G", "C"));
    ASSERT_TRUE(ambiguous("C", "G"));
    ASSERT_TRUE(ambiguous("T", "A"));
    ASSERT_FALSE(ambiguous("A", "G"));
    ASSERT_FALSE(ambiguous("G", "A"));
    ASSERT_FALSE(ambiguous("C", "T"));
    ASSERT_FALSE(ambiguous("T", "G"));
    ASSERT_FALSE(ambiguous("A", "C"));
    ASSERT_FALSE(ambiguous("G", "T"));
    ASSERT_FALSE(ambiguous("C", "A"));
    ASSERT_FALSE(ambiguous("T", "C"));
    ASSERT_FALSE(ambiguous("A", ""));
    ASSERT_FALSE(ambiguous("G", ""));
    ASSERT_FALSE(ambiguous("C", ""));
    ASSERT_FALSE(ambiguous("T", ""));
    ASSERT_FALSE(ambiguous("A", "TA"));
    ASSERT_FALSE(ambiguous("G", "CG"));
    ASSERT_FALSE(ambiguous("C", "GC"));
    ASSERT_FALSE(ambiguous("T", "AT"));
}

TEST_F(GENOTYPE_BASIC, CATEGORY)
{
    unsigned long long category;
    double pthres;
    // anything less than lowest threshold is consider 0
    PThresholding mock;
    mock.lower = 0.05;
    mock.inter = 0.01;
    mock.upper = 0.5;
    mock.no_full = true;
    double p_value = 0;
    category = calculate_category(p_value, pthres, mock);
    ASSERT_EQ(category, 0);
    ASSERT_DOUBLE_EQ(pthres, 0.05);
    // anything above the upper threshold is considered as 1.0 and with category
    // higher than the biggest category
    // though in theory, this should never happen as we will always filter SNPs
    // that are bigger than the last required threshold
    // the pthres is meaningless in this situation
    p_value = 0.6;
    category = calculate_category(p_value, pthres, mock);
    ASSERT_GT(category, mock.upper / mock.inter - mock.lower / mock.inter);
    // if we want full model, we will still do the same
    mock.no_full = false;
    category = calculate_category(p_value, pthres, mock);
    ASSERT_GT(category, mock.upper / mock.inter - mock.lower / mock.inter);
    ASSERT_DOUBLE_EQ(pthres, 1);
    mock.no_full = true;
    p_value = 0.05;
    category = calculate_category(p_value, pthres, mock);
    // this should be the first threshold
    ASSERT_EQ(category, 0);
    ASSERT_DOUBLE_EQ(pthres, 0.05);
    p_value = 0.055;
    category = calculate_category(p_value, pthres, mock);
    // this should be the first threshold
    ASSERT_EQ(category, 1);
    ASSERT_DOUBLE_EQ(pthres, 0.06);
}

void category_threshold(unsigned long long category,
                        unsigned long long expected_category, double pthres,
                        double expected_pthres)
{
    ASSERT_EQ(category, expected_category);
    ASSERT_DOUBLE_EQ(pthres, expected_pthres);
}
TEST_F(GENOTYPE_BASIC, BAR_LEVELS)
{
    unsigned long long category;
    double pthres;
    std::vector<double> barlevels = {0.001, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5};
    // anything less than lowest threshold is consider 0
    category = calculate_category(0.0, barlevels, pthres);
    category_threshold(category, 0, pthres, 0.001);
    category = calculate_category(0.001, barlevels, pthres);
    category_threshold(category, 0, pthres, 0.001);
    category = calculate_category(0.5, barlevels, pthres);
    category_threshold(category, 6, pthres, 0.5);
    category = calculate_category(0.7, barlevels, pthres);
    category_threshold(category, 7, pthres, 1);
}

TEST_F(GENOTYPE_BASIC, SNP_EXTRACTION)
{
    Reporter reporter(std::string("LOG"), 60, true);
    m_reporter = &reporter;
    std::ofstream dummy("DUMMY");
    std::vector<std::string> expected = {"rs1234", "rs5467", "rs8769"};
    for (auto e : expected) { dummy << e << std::endl; }
    dummy.close();
    ASSERT_TRUE(m_exclude_snp);
    m_snp_selection_list.clear();
    snp_extraction("DUMMY", "");
    ASSERT_FALSE(m_exclude_snp);
    ASSERT_EQ(m_snp_selection_list.size(), expected.size());
    for (auto e : expected)
    {
        ASSERT_TRUE(m_snp_selection_list.find(e) != m_snp_selection_list.end());
    }
    m_snp_selection_list.clear();
    m_exclude_snp = true;
    snp_extraction("", "DUMMY");
    ASSERT_TRUE(m_exclude_snp);
    ASSERT_EQ(m_snp_selection_list.size(), expected.size());
    for (auto e : expected)
    {
        ASSERT_TRUE(m_snp_selection_list.find(e) != m_snp_selection_list.end());
    }
    std::remove("DUMMY");
}

TEST_F(GENOTYPE_BASIC, LOAD_REF)
{
    Reporter reporter(std::string("LOG"), 60, true);
    m_reporter = &reporter;
    std::ofstream dummy("DUMMY");
    std::vector<std::string> expected = {"ID1 ID1", "ID2 FID", "ID5 ABD"};
    for (auto e : expected) { dummy << e << std::endl; }
    dummy.close();

    m_delim = "_";
    bool ignore = true;
    auto res = load_ref("DUMMY", !ignore);
    ASSERT_EQ(res.size(), expected.size());
    std::vector<std::string> token;
    for (auto e : expected)
    {
        token = misc::split(e);
        auto id = token[0] + m_delim + token[1];
        ASSERT_TRUE(res.find(id) != res.end());
    }
    // now try with ignore
    res.clear();
    m_delim = "a";
    res = load_ref("DUMMY", ignore);
    ASSERT_EQ(res.size(), expected.size());
    for (auto e : expected)
    {
        token = misc::split(e);
        ASSERT_TRUE(res.find(token[0]) != res.end());
    }
    // can't do without ignore if we only have one column
    std::remove("DUMMY");
    expected.clear();
    expected = {"ID1", "ID2", "ID3"};
    dummy.open("DUMMY");
    for (auto e : expected) { dummy << e << std::endl; }
    dummy.close();
    try
    {
        res = load_ref("DUMMY", !ignore);
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    std::remove("DUMMY");
}

TEST_F(GENOTYPE_BASIC, INIT_SAMPLE_VECTOR)
{
    // should fail as m_unfiltered_sample_ct is 0
    try
    {
        init_sample_vectors();
        FAIL();
    }
    catch (...)
    {
        SUCCEED();
    }
    // should sucess
    m_unfiltered_sample_ct = 1025;
    init_sample_vectors();
    ASSERT_EQ(m_num_male, 0);
    ASSERT_EQ(m_num_female, 0);
    ASSERT_EQ(m_num_ambig, 0);
    ASSERT_EQ(m_num_non_founder, 0);
    ASSERT_EQ(m_sample_ct, 0);
    ASSERT_EQ(m_founder_ct, 0);
    ASSERT_TRUE(!m_sample_for_ld.empty());
    ASSERT_TRUE(!m_calculate_prs.empty());
}


TEST_F(GENOTYPE_BASIC, SAMPLE_GENERATION)
{
    /* Check the following:
     * 1. Founder
     * 2. Non-founder
     * 3. Non-founder but keep
     * 4. Remove sample
     * 5. Keep sample
     * 6. Sex isn't too important here. Will just make sure we don't fail when
     * sex is not provided
     */
    // we will first assume fam format for easier handling
    // for bgen, the founder info structure should be empty
    //{"FID", "IID", "Dad",
    //"Mum", "Sex", "Pheno"}
    m_delim = " ";
    std::unordered_set<std::string> founder_info = {"FAM1 DAD1", "FAM1 MUM1",
                                                    "FAM2 MUM2", "FAM3 DAD1"};
    m_sample_selection_list.clear();
    m_sample_selection_list.insert("REMOVE1 REMOVE1");
    m_sample_selection_list.insert("REMOVE2 REMOVE2");
    m_remove_sample = true;
    std::vector<Sample_ID> result;
    std::unordered_set<std::string> processed_sample;
    std::vector<std::string> duplicated_sample;
    size_t cur_idx = 0;
    // must first initialize the vector
    m_unfiltered_sample_ct = 65;
    init_sample_vectors();
    const bool in_regress = true, cal_prs = true, cal_ld = true, keep = true,
               duplicated = true;
    // now start testing, this is a founder;
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"ID1", "ID1", "0", "0", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // now non-founder, but not using it for regression
    sample_generation_check(
        founder_info, !in_regress, cal_prs, !cal_ld, keep, !duplicated,
        std::vector<std::string> {"FAM1", "BOY1", "DAD1", "MUM1", "1", "0"},
        processed_sample, result, duplicated_sample, cur_idx);
    // now if we change it to keep non_founder
    m_keep_nonfounder = true;
    // will use it for everything except LD
    sample_generation_check(
        founder_info, in_regress, cal_prs, !cal_ld, keep, !duplicated,
        std::vector<std::string> {"FAM2", "GIRL1", "DAD2", "MUM2", "2", "0"},
        processed_sample, result, duplicated_sample, cur_idx);
    // another non-founder, but both parents are not found in the system
    // so should treat as a normal founders
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"FAM4", "GIRL2", "DAD4", "MUM4", "2", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // should be ok if we are in the same family as a founder but not the same
    // parent (e.g. skipped a generation?)
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"FAM3", "BOY2", "0", "MUM2", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // and if we have any parent founder, we are defined as non-founder
    sample_generation_check(
        founder_info, in_regress, cal_prs, !cal_ld, keep, !duplicated,
        std::vector<std::string> {"FAM1", "BOY3", "0", "MUM1", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // sample removal
    sample_generation_check(
        founder_info, in_regress, cal_prs, !cal_ld, !keep, !duplicated,
        std::vector<std::string> {"REMOVE1", "REMOVE1", "0", "MUM1", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // must match both IID and FID to remove it
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"REMOVE1", "BOY4", "0", "MUM1", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
    // now switch to keep
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"REMOVE2", "REMOVE2", "0", "MUM1", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx, false);
    // test ignore FID
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, !duplicated,
        std::vector<std::string> {"ABC", "ID2", "0", "0", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx, true, true);
    // now check duplicated sample
    sample_generation_check(
        founder_info, in_regress, cal_prs, cal_ld, keep, duplicated,
        std::vector<std::string> {"ID1", "ID1", "0", "0", "1", "1"},
        processed_sample, result, duplicated_sample, cur_idx);
}

#endif // GENOTYPE_TEST_HPP
