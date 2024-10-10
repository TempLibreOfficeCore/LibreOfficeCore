/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cassert>
#include <iostream>

#include <sal/config.h>

#include <cppuhelper/exc_hlp.hxx>
#include <com/sun/star/ucb/InteractiveAugmentedIOException.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <sfx2/app.hxx>
#include <vcl/svapp.hxx>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestProgressListener.h>

#import <LibreOfficeKit/LibreOfficeKit.hxx>

extern "C" {
#import <native-code.h>
}

#include <premac.h>
#import <CoreGraphics/CoreGraphics.h>
#import "ViewController.h"
#include <postmac.h>


#define CPPUNIT_PLUGIN_EXPORTED_NAME CppuMisc
#include "../../../cppuhelper/qa/misc/test_misc.cxx"

#undef CPPUNIT_PLUGIN_EXPORTED_NAME
#define CPPUNIT_PLUGIN_EXPORTED_NAME CppuUnourl
#include "../../../cppuhelper/qa/unourl/cppu_unourl.cxx"

#undef CPPUNIT_PLUGIN_EXPORTED_NAME
#define CPPUNIT_PLUGIN_EXPORTED_NAME BitmapTest
#include "../../../vcl/qa/cppunit/BitmapTest.cxx"

#define main tilebench_main
#include "../../../libreofficekit/qa/tilebench/tilebench.cxx"


@interface ViewController()
@property(nonatomic,assign)int m_convert_flag;
@property(nonatomic,strong)UILabel *m_msg_label;
@end

@implementation ViewController

LibreOfficeKit *lo_kit;
Office *office;


void officeCallback(int nType, const char* pPayload, void* pData)
{
    NSLog(@"yantao-doc officeCallback ====执行==nType %d ,pPayload  %s",nType,pPayload);
    if(LOK_DOCUMENT_CONVERT == nType)
    {
        NSLog(@"yantao-doc 文件转换进度   %s",pPayload);
        
        @autoreleasepool {
            NSString *str = [NSString stringWithCString:pPayload encoding:NSUTF8StringEncoding];
            [[NSNotificationCenter defaultCenter] postNotificationName:@"DocumentConvertNotice" object:str];

        }
    }
}

- (BOOL)checkDocument:(LibreOfficeKitDocument *)document office:(Office *)office
{
    if(document == nullptr || office->getError() == nullptr)
    {
        printf("\ncheckDocument error\n");
        return NO;
    }
    
    long nWidth;
    long nHeight;
    document->pClass->getDocumentSize(document, &nWidth, &nHeight);
    NSLog(@"document width %ld,nHeight %ld",nWidth,nHeight);
    
    if(nWidth == 0 || nHeight == 0)
    {
        return NO;
    }
    return YES;
}


- (void)showDocumentConvertInfo:(NSNotification*)notice
{
    dispatch_async(dispatch_get_main_queue(), ^{
        self.m_msg_label.text = [NSString stringWithFormat:@"文件转换进度 %@",notice.object];
    });
}



- (void)viewDidLoad 
{
    [super viewDidLoad];
    
    self.m_convert_flag = 0;
    char *lang = std::getenv("LANG");
    NSString *app_locale;
    if (lang != nullptr)
    {
        app_locale = [NSString stringWithUTF8String:lang];
        NSLog(@"getenv app_locale %@",app_locale);

    }
    else
    {
        app_locale = [[NSLocale preferredLanguages] firstObject];
        NSLog(@"app_locale %@",app_locale);

    }



    self.view.backgroundColor = [UIColor whiteColor];
    NSArray *caches_dirs = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cachdir  = [caches_dirs firstObject];
    
    

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(showDocumentConvertInfo:) name:@"DocumentConvertNotice" object:nil];
        
    


    //lo_kit->pClass->setDocumentPassword(lo_kit,[url UTF8String],[pwd UTF8String]);
    {
        // First run some normal cppunit tests. Seems that at least the BitmapTest needs to be run
        // with the SolarMutex held.

        /*SolarMutexGuard solarMutexGuard;

        CppUnit::TestResult result;

        CppUnit::TextTestProgressListener logger;
        result.addListener(&logger);

        CppUnit::TestResultCollector collector;
        result.addListener(&collector);

        CppUnit::TestRunner runner;
        runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
        runner.run(result);

        CppUnit::CompilerOutputter outputter(&collector, std::cerr);
        outputter.write();*/
    }
    
    

    // Then some more specific stuff
    //tilebench_main(0, nullptr);
    
    lo_kit = lok_init_2(nullptr, nullptr);
//    const char* env_str = "SAL_LOG=+WARN+INFO";
//    char* env = new char[20]; // Allocate memory for 20 characters
//    strcpy(env, env_str); // Copy the string into the allocated memory
//    printf("\nenv %s\n",env);
//    putenv(env);
//    delete[] env;

    
    
    NSString *pwd = @"123456";
    lo_kit->pClass->registerCallback(lo_kit,officeCallback,nullptr);
    char*  type = lo_kit->pClass->getFilterTypes(lo_kit);
    printf("\ntype %s\n",type);

    char *version = lo_kit->pClass->getVersionInfo(lo_kit);
    const char *error = lo_kit->pClass->getError(lo_kit);
    
    office = new Office(lo_kit);
    printf("\nversion %s ,error %s\n",version,error);
    
    // UIStackView
    UIStackView *stackView = [[UIStackView alloc] init];
    stackView.backgroundColor = UIColor.redColor;
    stackView.frame = CGRectMake(0, 100, self.view.frame.size.width, 120);
    stackView.axis = UILayoutConstraintAxisVertical;
    // 水平方向
    stackView.distribution = UIStackViewDistributionFillEqually;
    // 垂直方向
    stackView.alignment = UIStackViewAlignmentFill;
    stackView.spacing = 8;
    UIButton *btn = [[UIButton alloc] init];
    btn.backgroundColor = [UIColor greenColor];
    [btn setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [btn setTitle:@"文件转换" forState:UIControlStateNormal];
    [btn addTarget:self action:@selector(convertFile:) forControlEvents:UIControlEventTouchUpInside];

    [stackView addArrangedSubview:btn];
    
    
    UIButton *btn2 = [[UIButton alloc] init];
    btn2.backgroundColor = [UIColor yellowColor];
    [btn2 setTitleColor:UIColor.blackColor forState:UIControlStateNormal];
    [btn2 setTitle:@"取消转换" forState:UIControlStateNormal];
    [btn2 addTarget:self action:@selector(cancelConvertFile:) forControlEvents:UIControlEventTouchUpInside];

    [stackView addArrangedSubview:btn2];
    
    
    [self.view addSubview:stackView];
    
    
    UILabel *msg_label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.view.frame.size.width*0.8, 100)];
    self.m_msg_label = msg_label;
    msg_label.center = self.view.center;
    msg_label.backgroundColor = [UIColor clearColor];
    msg_label.textColor = [UIColor blueColor];
    msg_label.numberOfLines = 0;
    msg_label.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:msg_label];
    msg_label.text = @"加载进度";
    
    
    
 
    // 添加子组件
    /*UIView *view1 = [[UIView alloc] init];
    view1.backgroundColor = UIColor.grayColor;
    UIView *view2 = [[UIView alloc] init];
    view2.backgroundColor = UIColor.yellowColor;
    UIView *view3 = [[UIView alloc] init];
    view3.backgroundColor = UIColor.blueColor;
    UIView *view4 = [[UIView alloc] init];
    view4.backgroundColor = UIColor.blackColor;

    // 必须使用的特殊添加方式
    [stackView addArrangedSubview:view1];
    [stackView addArrangedSubview:view2];
    [stackView addArrangedSubview:view3];
    [stackView addArrangedSubview:view4];*/

}

/*{
    [super viewDidLoad];

    // Simplest (?) way to do all the tedious initialization
    lok_init_2(nullptr, nullptr);

    {
        // First run some normal cppunit tests. Seems that at least the BitmapTest needs to be run
        // with the SolarMutex held.

        SolarMutexGuard solarMutexGuard;

        CppUnit::TestResult result;

        CppUnit::TextTestProgressListener logger;
        result.addListener(&logger);

        CppUnit::TestResultCollector collector;
        result.addListener(&collector);

        CppUnit::TestRunner runner;
        runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
        runner.run(result);

        CppUnit::CompilerOutputter outputter(&collector, std::cerr);
        outputter.write();
    }

    // Then some more specific stuff
    tilebench_main(0, nullptr);
}*/

- (void)cancelConvertFile:(id)sender
{
    NSLog(@"yantao-doc 取消文件转换");
    
    lo_kit->pClass->stopSaveAs(nullptr);
}
- (void)convertFile:(id)sender
{
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        self.m_convert_flag++;
        NSString *url = [[NSBundle mainBundle] pathForResource:@"XSpirit2" ofType:@"docx"];//test2
//         NSString *url = [[NSBundle mainBundle] pathForResource:@"testdoc" ofType:@"docx"];
  
        if(self.m_convert_flag  == 1)
        {
            url = [[NSBundle mainBundle] pathForResource:@"test2" ofType:@"docx"];
        }
        else if(self.m_convert_flag  == 2)
        {
            url = [[NSBundle mainBundle] pathForResource:@"test_large" ofType:@"pptx"];
        }
        else if(self.m_convert_flag  >= 3 && self.m_convert_flag  < 6)
        {
            url = [[NSBundle mainBundle] pathForResource:@"video_to_docx" ofType:@"docx"];
        }
        else if(self.m_convert_flag  >= 5)
        {
            url = [[NSBundle mainBundle] pathForResource:@"testPPT" ofType:@"pptx"];
        }
        else if(self.m_convert_flag  >= 6)
        {
            url = [[NSBundle mainBundle] pathForResource:@"testdoc" ofType:@"docx"];
        }
        
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSError *error = nil;
        NSDictionary *attributes = [fileManager attributesOfItemAtPath:url error:&error];

        if (!error) {
            NSString *fileType = attributes[NSFileType];
            NSLog(@"File type: %@", fileType);
        } else {
            NSLog(@"Error getting file attributes: %@", error.localizedDescription);
        }

        NSLog(@"转换文件路径 : %@",url);

        
        //设置延迟检测函数
        dispatch_block_t block = dispatch_block_create(DISPATCH_BLOCK_BARRIER, ^{
            NSLog(@"ERROR  %@ 格式不支持",url);
            //LibreOfficeKitDocument *document = lo_kit->pClass->documentLoad(lo_kit,[url UTF8String]);
            
//            lo_kit->pClass->destroy(lo_kit);
//            if(document == nullptr)
//            {
//                NSLog(@"document格式不支持 加载失败===============");
//            }
//            else
//            {
//                NSLog(@"document格式不支持 加载成功===============");
//            }

        });
        
        //延迟监测
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(30 * NSEC_PER_SEC)), dispatch_get_global_queue(0, 0),block );
        
        //office->setOptionalFeatures(LOK_FEATURE_DOCUMENT_PASSWORD);
        LibreOfficeKitDocument *document = lo_kit->pClass->documentLoad(lo_kit,[url UTF8String]);
        
        dispatch_cancel(block);
        
        if(document == nullptr)
        {
            NSLog(@"document加载失败===============");
        }
        else
        {
            NSLog(@"document 加载成功===============");
            
            document->pClass->initializeForRendering(document,nullptr);

            
            long nWidth;
            long nHeight;
            document->pClass->getDocumentSize(document, &nWidth, &nHeight);
            NSLog(@"document width %ld,nHeight %ld",nWidth,nHeight);

        }
        if(document == nullptr)
        {
            NSLog(@"document url %@ 格式不支持",url);

            return;
        }
        
        document->pClass->setPart(document,0);
    //    dispatch_after(5, dispatch_get_main_queue(), ^{
    //
    //    });
        
        NSString *pdf_path = [self createFile:[NSString stringWithFormat:@"yantao_office_ppt_%@.pdf",@([[NSDate date] timeIntervalSince1970])]];
        int res = document->pClass->saveAs(document,[pdf_path UTF8String],"pdf","");
        
        document->pClass->destroy(document);
        NSLog(@"document->pClass->saveAs pdf res %d, pdf_path %@ ,url %@",res,pdf_path,url);
        
        NSLog(@"yantao-doc 文件转换进度 转换完成 %@",pdf_path);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            self.m_msg_label.text = [NSString stringWithFormat:@"文件转换进度 转换完成 %@",pdf_path];

        });
    });
    
}

- (NSString*)createFile:(NSString*)fileName
{
    // 获取Documents目录路径
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];

    // 指定要创建的文件路径
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent:fileName];

    // 检查文件是否已存在，如果不存在则创建文件
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:filePath]) {
        BOOL success = [@"Initial content for the file" writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
        if (success) {
            NSLog(@"文件创建成功：%@", filePath);
        } else {
            NSLog(@"创建文件失败");
        }
    } else {
        NSLog(@"文件已存在：%@", filePath);
    }
    
    return filePath;

}

@end
