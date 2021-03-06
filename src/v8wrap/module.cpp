#include <v8wrap/module.hpp>
#include <v8wrap/exception.hpp>
#include <v8wrap/core.hpp>
#include <v8wrap/value.hpp>

#include <stdexcept>
#include <mutex>

namespace 
{
    // helper type for the visitor #4
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    // explicit deduction guide (not needed as of C++20)
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

namespace v8wrap
{
    const static std::vector<std::string> stati{"kUninstantiated", "kInstantiating", "kInstantiated", "kEvaluating", "kEvaluated" , "kErrored"};
//#####################################################################################################################
    struct Module::Implementation
    {
        v8::Local<v8::Context> context;
        v8::ScriptOrigin origin;
        v8::ScriptCompiler::Source source;
        v8::Context::Scope contextScope;
        Module::CreationParameters::onModuleLoadFunction moduleLoader;
        // synthetic only:
        ExportsContainer exports;
        v8::Local<v8::Module> mod;
        std::once_flag initOnce;
        bool synthetic;

        Implementation(Module::CreationParameters&& params);
        Implementation(Module::SyntheticCreationParameters&& params);
    };
//---------------------------------------------------------------------------------------------------------------------
    Module::Implementation::Implementation(Module::CreationParameters&& params)
        : context{params.context}
        , origin{
            v8::String::NewFromUtf8(params.context->GetIsolate(), params.fileName.c_str()).ToLocalChecked(),      // specifier
            v8::Integer::New(params.context->GetIsolate(), 0),             // line offset
            v8::Integer::New(params.context->GetIsolate(), 0),             // column offset
            v8::False(params.context->GetIsolate()),                       // is cross origin
            v8::Local<v8::Integer>(),                     // script id
            v8::Local<v8::Value>(),                       // source map URL
            v8::False(params.context->GetIsolate()),                       // is opaque
            v8::False(params.context->GetIsolate()),                       // is WASM
            v8::True(params.context->GetIsolate())                         // is ES6 module
        }
        , source{
            v8::String::NewFromUtf8(params.context->GetIsolate(), params.script.c_str(), v8::NewStringType::kNormal).ToLocalChecked(), 
            origin
        }
        , contextScope{context}
        , moduleLoader{std::move(params.onModuleLoad)}
        , exports{}
        , mod{[this]() {
            v8::TryCatch exc(context->GetIsolate());
            v8::Local<v8::Module> mod_;
            auto step = v8::ScriptCompiler::CompileModule(context->GetIsolate(), &source);
            rethrowException(context->GetIsolate(), exc);
            if (!step.ToLocal(&mod_))
                rethrowExceptionFallback(context->GetIsolate(), exc, "Could not compile module.");
            return mod_;
        }()}
        , initOnce{}
        , synthetic{false}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    Module::Implementation::Implementation(Module::SyntheticCreationParameters&& params)
        : context{params.context}
        , origin{
            v8::String::NewFromUtf8(params.context->GetIsolate(), params.fileName.c_str()).ToLocalChecked(),      // specifier
            v8::Integer::New(params.context->GetIsolate(), 0),             // line offset
            v8::Integer::New(params.context->GetIsolate(), 0),             // column offset
            v8::False(params.context->GetIsolate()),                       // is cross origin
            v8::Local<v8::Integer>(),                     // script id
            v8::Local<v8::Value>(),                       // source map URL
            v8::False(params.context->GetIsolate()),                       // is opaque
            v8::False(params.context->GetIsolate()),                       // is WASM
            v8::True(params.context->GetIsolate())                         // is ES6 module
        }
        , source{
            v8::String::NewFromUtf8(params.context->GetIsolate(), "", v8::NewStringType::kNormal).ToLocalChecked(), 
            origin
        }
        , contextScope{context}
        , moduleLoader{std::move(params.onModuleLoad)}
        , exports{params.exports}
        , mod{[this, &params]() {
            v8::TryCatch exc(context->GetIsolate());
            v8::Local<v8::Module> mod_ = v8::Module::CreateSyntheticModule(
                context->GetIsolate(),
                v8cast<std::string>(context, params.fileName),
                [&params]() {
                    std::vector <v8::Local<v8::String>> exps;
                    for (auto const& [key, value] : params.exports)
                        exps.push_back(v8cast<std::string>(params.context, key));
                    return exps;
                }(),
                [](v8::Local<v8::Context> ctx, v8::Local<v8::Module> m) -> v8::MaybeLocal<v8::Value>
                {
                    auto wrap = findModule(m);
                    if (wrap)
                    {
                        v8::TryCatch exc(ctx->GetIsolate());
                        for (auto&& exp : wrap->exports()) 
                        {
                            bool fail = false;
                            auto maybe = m->SetSyntheticModuleExport(
                                ctx->GetIsolate(), 
                                v8cast<std::string>(wrap->context(), exp.first), 
                                exp.second
                            );
                            rethrowException(ctx->GetIsolate(), exc);
                            fail = (maybe.IsNothing() || maybe.FromJust() == false);
                            // std::visit(overloaded{
                            //     [&exp, &ctx, &m, &wrap, &exc, &fail](v8::Local<v8::Value> exportValue)
                            //     {
                            //         auto maybe = m->SetSyntheticModuleExport(
                            //             ctx->GetIsolate(), 
                            //             v8cast<std::string>(wrap->context(), exp.first), 
                            //             exportValue
                            //         );
                            //         rethrowException(ctx->GetIsolate(), exc);
                            //         fail = (maybe.IsNothing() || maybe.FromJust() == false);
                            //     },
                            //     [&exp, &ctx, &m, &wrap, &exc, &fail](v8::Local<v8::ObjectTemplate> exportValue)
                            //     {
                            //         auto maybe = m->SetSyntheticModuleExport(
                            //             ctx->GetIsolate(), 
                            //             v8cast<std::string>(wrap->context(), exp.first), 
                            //             v8::Local<v8::Value>::New(ctx->GetIsolate(), exportValue)
                            //         );
                            //         rethrowException(ctx->GetIsolate(), exc);
                            //         fail = (maybe.IsNothing() || maybe.FromJust() == false);
                            //     },
                            //     [](auto){}
                            // }, exp.second);
                            if (fail)
                                return v8::False(ctx->GetIsolate());
                        };                  
                        return v8::True(ctx->GetIsolate());
                    }
                    return v8::False(ctx->GetIsolate());
                }
            );
            rethrowException(params.context->GetIsolate(), exc);
            return mod_;
        }()}
        , synthetic{true}
    {
    }
//#####################################################################################################################
    Module::Module(CreationParameters&& params)
        : impl_{std::make_unique<Implementation>(std::move(params))}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    Module::Module(SyntheticCreationParameters&& params)
        : impl_{std::make_unique<Implementation>(std::move(params))}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    std::vector<std::string> Module::getModuleRequests() const
    {
        std::vector <std::string> result;
        for (int i = 0; i < impl_->mod->GetModuleRequestsLength(); i++) {
            v8::Local<v8::String> specifier = impl_->mod->GetModuleRequest(i); // "some thing"
            result.push_back(v8cast<std::string>(impl_->context, specifier));
        }
        return result;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Module::setup()
    {
        std::call_once(impl_->initOnce, [this]()
        {
            registerModule(impl_->mod->GetIdentityHash(), weak_from_this());
            if (!impl_->synthetic)
            {
                v8::TryCatch exc(impl_->context->GetIsolate());
                auto maybe = impl_->mod->InstantiateModule(impl_->context, [](
                    v8::Local<v8::Context> ctx, // "main.mjs"
                    v8::Local<v8::String> specifier, // "some thing"
                    v8::Local<v8::Module> referrer
                ) -> v8::MaybeLocal<v8::Module> 
                {
                    auto m = findModule(referrer);
                    if (!m)
                        throw std::runtime_error("Module not found.");
                        //return v8::MaybeLocal<v8::Module>{};
                    v8::TryCatch exc(m->context()->GetIsolate());
            
                    auto maybeMod = m->impl_->moduleLoader(ctx, v8cast<std::string>(ctx, specifier));
                    if (!maybeMod.IsEmpty())
                        return maybeMod.ToLocalChecked();
                    rethrowException(m->context()->GetIsolate(), exc);
                    throw std::runtime_error("Cannot load module.");
                });
                rethrowException(impl_->context->GetIsolate(), exc);
                if (maybe.IsNothing() || maybe.FromJust() == false)
                    throw std::runtime_error("There was some error during module instatiation.");
                
                // setting this callback enables dynamic import
                // isolate->SetHostImportModuleDynamicallyCallback([](
                //     v8::Local<v8::Context> context,
                //     v8::Local<v8::ScriptOrModule> referrer,
                //     v8::Local<v8::String> specifier
                // ) -> v8::MaybeLocal<v8::Promise> 
                // {
                //     v8::Isolate* isolate = context->GetIsolate();
                //     v8::EscapableHandleScope handle_scope(isolate);
                    
                //     // Local<Value> result;
                //     // if (maybe_result.ToLocal(&result)) 
                //     // {
                //     //     return handle_scope.Escape(result.As<Promise>());
                //     // }
                //     //return v8::MaybeLocal<v8::Promise>();
                //     v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
                //     resolver->Resolve(context, v8::Local<v8::Module>{});
                //     return resolver->GetPromise();
                // });

                /*
                // setting this callback enables import.meta
                isolate->SetHostInitializeImportMetaObjectCallback([](Local<Context> context,
                                                                    Local<Module> module,
                                                                    Local<Object> meta) {
                // meta->Set(key, value); you could set import.meta.url here
                });*/
            }
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Context>& Module::context()
    {
        return impl_->context;
    }
//---------------------------------------------------------------------------------------------------------------------
    Module::~Module()
    {
        unregisterModule(impl_->mod->GetIdentityHash(), weak_from_this());
    }
//---------------------------------------------------------------------------------------------------------------------
    Module::Module(Module&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    Module& Module::operator=(Module&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Module> Module::getModule()
    {
        return impl_->mod;
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Value> Module::evaluate()
    {
        setup();
        v8::EscapableHandleScope handle_scope(impl_->context->GetIsolate());
       
        v8::TryCatch exc(impl_->context->GetIsolate());
        v8::Local<v8::Value> result;
        const auto maybeEval = impl_->mod->Evaluate(impl_->context);
        if (maybeEval.IsEmpty() || !maybeEval.ToLocal(&result))
            rethrowExceptionFallback(impl_->context->GetIsolate(), exc, "Unknown error in module.");
        rethrowException(impl_->context->GetIsolate(), exc);

        return handle_scope.Escape(result);
    }
//---------------------------------------------------------------------------------------------------------------------
    ExportsContainer& Module::exports()
    {
        return impl_->exports;
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Value> Module::getNamespace()
    {
        return impl_->mod->GetModuleNamespace();
    }
//#####################################################################################################################
}