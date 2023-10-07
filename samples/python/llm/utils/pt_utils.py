# -*- coding: utf-8 -*-
# Copyright (C) 2018-2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
from diffusers.pipelines import DiffusionPipeline
import timm
from utils.config_class import PT_MODEL_CLASSES_MAPPING, TOKENIZE_CLASSES_MAPPING, DEFAULT_MODEL_CLASSES
import os
import time

MAX_CONNECT_TIME = 50

def set_bf16(model, device, **kwargs):
    try:
        if len(kwargs['config']) > 0 and kwargs['config'].get('PREC_BF16') and kwargs['config']['PREC_BF16'] is True:
            model = model.to(device, dtype=torch.bfloat16)
            print("Set inference precision to bf16")
    except:
        print("Catch exception for setting inference precision to bf16.")
        raise RuntimeError(f"Set prec_bf16 fail.")
    return model

def get_text_model_from_huggingface(model_path, connect_times, **kwargs):
    model_type = DEFAULT_MODEL_CLASSES[kwargs['use_case']]
    model_class = PT_MODEL_CLASSES_MAPPING[model_type]
    token_class = TOKENIZE_CLASSES_MAPPING[model_type]
    from_pretrain_time = 0
    try:
        start = time.perf_counter()
        tokenizer = token_class.from_pretrained(kwargs['model_id'])
        model = model_class.from_pretrained(kwargs['model_id'])
        end = time.perf_counter()
        from_pretrain_time = end - start
        print(f"get tokenizer and model from huggingface success")
        tokenizer.save_pretrained(model_path)
        model.save_pretrained(model_path)
    except:
        print(f"try to connect huggingface times: {connect_times}....")
        if connect_times > MAX_CONNECT_TIME:
            raise RuntimeError(f"==Failure ==: connect times {MAX_CONNECT_TIME}, connect huggingface failed")
        time.sleep(3)
        connect_times += 1
        model, tokenizer, from_pretrain_time = get_text_model_from_huggingface(model_path, connect_times, **kwargs)
    return model, tokenizer, from_pretrain_time

def create_text_gen_model(model_path, device, **kwargs):
    model_path = Path(model_path)
    from_pretrain_time = 0
    if model_path.exists():
        if model_path.is_dir():
            # Checking if the list is empty or not
            if len(os.listdir(model_path)) == 0:
                if kwargs['model_id'] != "":
                    print(f"get text model from huggingface...")
                    connect_times = 1
                    model, tokenizer, from_pretrain_time = get_text_model_from_huggingface(model_path, connect_times, **kwargs)
                else:
                    raise RuntimeError(f"==Failure ==: the model id of huggingface should not be empty!")
            else:

                print(f"load text model from model path:{model_path}")
                default_model_type = DEFAULT_MODEL_CLASSES[kwargs['use_case']]
                model_type = kwargs.get("model_type" , default_model_type)
                model_class = PT_MODEL_CLASSES_MAPPING.get(model_type, PT_MODEL_CLASSES_MAPPING[default_model_type])
                token_class = TOKENIZE_CLASSES_MAPPING.get(model_type, TOKENIZE_CLASSES_MAPPING[default_model_type])
                start = time.perf_counter()
                if model_type == "chatglm":
                    model = model_class.from_pretrained(model_path, trust_remote_code=True).to("cpu", dtype=float)
                else:
                    model = model_class.from_pretrained(model_path, trust_remote_code=True)
                tokenizer = token_class.from_pretrained(model_path, trust_remote_code=True)
                end = time.perf_counter()
                from_pretrain_time = end - start
        else:
            raise RuntimeError(f"==Failure ==: model path:{model_path} is not directory")
    else:
        raise RuntimeError(f"==Failure ==: model path:{model_path} is not exist")

    print(f"model path:{model_path}, from pretrained time: {from_pretrain_time:.2f}s")

    if device is not None:
        GPTJFCLM="transformers.models.gptj.modeling_gptj.GPTJForCausalLM"
        LFCLM="transformers.models.llama.modeling_llama.LlamaForCausalLM"
        BFCLM="transformers.models.bloom.modeling_bloom.BloomForCausalLM"
        GPT2LMHM="transformers.models.gpt2.modeling_gpt2.GPT2LMHeadModel"
        GPTNEOXCLM="transformers.models.gpt_neox.modeling_gpt_neox.GPTNeoXForCausalLM"
        ChatGLMFCG="transformers_modules.pytorch_original.modeling_chatglm.ChatGLMForConditionalGeneration"
        REAL_BASE_MODEL_NAME=str(type(model))
        print("Real base model=", REAL_BASE_MODEL_NAME)
        # BFCLM will trigger generate crash.
        if any([x in REAL_BASE_MODEL_NAME for x in [GPTJFCLM, LFCLM, BFCLM, GPT2LMHM, GPTNEOXCLM, ChatGLMFCG]]):
            model = set_bf16(model, device, **kwargs)
        else:
            if len(kwargs['config']) > 0 and kwargs['config'].get('PREC_BF16') and kwargs['config']['PREC_BF16'] is True:
                print("Param [bf16/prec_bf16] will not work.")
            model.to(device)
    else:
        raise RuntimeError("==Failure ==: no device to load")
    return model, tokenizer, from_pretrain_time

def get_image_model_from_huggingface(model_path, connect_times, **kwargs):
    model_type = DEFAULT_MODEL_CLASSES[kwargs['use_case']]
    model_class = PT_MODEL_CLASSES_MAPPING[model_type]
    from_pretrain_time = 0
    try:
        start = time.perf_counter()
        pipe = model_class.from_pretrained(kwargs['model_id'])
        pipe.save_pretrained(model_path)
        end = time.perf_counter()
        from_pretrain_time = end - start
        print(f"get image model from huggingface success")
    except:
        print(f"try to connect huggingface times: {connect_times}....")
        if connect_times > MAX_CONNECT_TIME:
            raise RuntimeError(f"==Failure ==: connect times {MAX_CONNECT_TIME}, connect huggingface failed")
        time.sleep(3)
        connect_times += 1
        pipe, from_pretrain_time = get_image_model_from_huggingface(model_path, connect_times, **kwargs)
    return pipe, from_pretrain_time

def create_image_gen_model(model_path, device, **kwargs):
    model_path = Path(model_path)
    from_pretrain_time = 0
    if model_path.exists():
        if model_path.is_dir():
            # Checking if the list is empty or not
            if len(os.listdir(model_path)) == 0:
                if kwargs['model_id'] != "":
                    print(f"get image model from huggingface...")
                    connect_times = 1
                    pipe, from_pretrain_time = get_image_model_from_huggingface(model_path, connect_times, **kwargs)
                else:
                    raise RuntimeError(f"==Failure ==: the model id of huggingface should not be empty!")
            else:
                print(f"load image model from model path:{model_path}")
                model_type = DEFAULT_MODEL_CLASSES[kwargs['use_case']]
                model_class = PT_MODEL_CLASSES_MAPPING[model_type]
                start = time.perf_counter()
                pipe = model_class.from_pretrained(model_path)
                end = time.perf_counter()
                from_pretrain_time = end - start
        else:
            raise RuntimeError(f"==Failure ==: model path:{model_path} is not directory")
    else:
        raise RuntimeError(f"==Failure ==: model path:{model_path} is not exist")
    
    print(f"model path:{model_path}, from pretrained time: {from_pretrain_time:.2f}s")

    if device is not None:
        pipe.to(device)
    else:
        raise RuntimeError("==Failure ==: no device to load")
    return pipe, from_pretrain_time

def create_image_classification_model(model_path, device, **kwargs):
    model_path = Path(model_path)
    model_file = None
    model_id = None
    if model_path.exists():
        if model_path.is_dir():
            model_file = list(model_path.glob("*.pth"))
            if model_file:
                model_file = model_file[0]
            else:
                model_file = None
            model_id = model_path.name
        else:
            model_file = model_path
            model_id = model_path.name.replace(".pth", "")
    else:
        model_id = model_path.name.replace(".pth", "")
    model = timm.create_model(model_id, pretrained=model_file is None)
    if model_file:
        model.load_state_dict(torch.load(model_file, map_location="cpu"))
    else:
        print(model.state_dict())
        torch.save(model.state_dict(), model_path / f"{model_id}.pth")
    if device:
        model.to(device)
    else:
        raise RuntimeError("==Failure ==: no device to load")
    model.eval()
    data_config = timm.data.resolve_data_config([], model=model_id, use_test_size=True)
    input_size = (1, ) + data_config["input_size"]
    return model, input_size

def create_ldm_super_resolution_model(model_path, device, **kwargs):
    model_path = Path(model_path)
    from_pretrain_time = 0
    if model_path.exists():
        if model_path.is_dir():
            # Checking if the list is empty or not
            if len(os.listdir(model_path)) == 0:
                if kwargs['model_id'] != "":
                    print(f"get super resolution model from huggingface...")
                    connect_times = 1
                    pipe, from_pretrain_time = get_image_model_from_huggingface(model_path, connect_times, **kwargs)
                else:
                    raise RuntimeError(f"==Failure ==: the model id of huggingface should not be empty!")
            else:
                print(f"load image model from model path:{model_path}")
                model_type = DEFAULT_MODEL_CLASSES[kwargs['use_case']]
                model_class = PT_MODEL_CLASSES_MAPPING[model_type]
                start = time.perf_counter()
                pipe = model_class.from_pretrained(model_path)
                end = time.perf_counter()
                from_pretrain_time = end - start
        else:
            raise RuntimeError(f"==Failure ==: model path:{model_path} is not directory")
    else:
        raise RuntimeError(f"==Failure ==: model path:{model_path} is not exist")

    print(f"model path:{model_path}, from pretrained time: {from_pretrain_time:.2f}s")

    if device is not None:
        pipe.to(device)
    else:
        raise RuntimeError("==Failure ==: no device to load")
    return pipe, from_pretrain_time