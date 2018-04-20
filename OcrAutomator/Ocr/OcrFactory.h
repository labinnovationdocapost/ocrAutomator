#pragma once
#include "Base/ImageFormatEnum.h"
#include <vector>
#include <rttr/type>
#include "Api/OcrProperty.h"

namespace Docapost {
	namespace IA {
		namespace Tesseract {
			// Formard declaration pour evité les includes incluant l'ensemble du programme
			class Ocr;

			class OcrFactory
			{
			protected:
				ImageFormatEnum mImageFormat = ImageFormatEnum::JPG;
				std::vector<std::string> mExtension = { ".txt" };


				static std::list<OcrProperty*> _GetOcrParametersDefinition(rttr::type classs_type)
				{
					auto props = classs_type.get_properties();

					std::list<OcrProperty*> ret;
					for (auto& prop : props)
					{
						OcrProperty* stru = new OcrProperty();
						ret.push_back(stru);

						stru->name = prop.get_name();
						stru->typeName = prop.get_type().get_name();
						stru->isEnum = prop.get_type().is_enumeration();
						if (prop.get_metadata("Description"))
							stru->description = prop.get_metadata("Description").get_value<std::string>();
						if (stru->isEnum)
						{
							for (auto e : prop.get_type().get_enumeration().get_values())
							{
								stru->enu.push_back(e.to_string());
							}
						}
					}
					return ret;
				}
			public:

				/**
				* \brief Liste l'ensemble des moteur d'OCR disponible sur cette instance
				* \return liste des noms des moteur d'ocr disponible
				*/
				static std::vector<std::string> GetOcrs()
				{
					auto classs_type = rttr::type::get_by_name("OcrFactory").get_derived_classes();
					std::vector<std::string> ret;
					for (auto& type : classs_type)
					{
						ret.push_back(type.get_name());
					}
					return ret;
				}

				/**
				* \brief Determine l'existance d'un moteur OCR
				* \param ocr nom de l'OCR a trouver
				* \return true si l'OCR existe
				*/
				static bool OcrExist(std::string ocr)
				{
					boost::algorithm::to_lower(ocr);
					ocr[0] = toupper(ocr[0]);
					rttr::type classs_type = rttr::type::get_by_name(ocr.c_str());
					if (classs_type)
						return true;

					return false;
				}

				/**
				* \brief créer une instance du moteur d'OCR
				* \param ocr nom de l'OCR
				*/
				static OcrFactory* CreateNew(std::string ocr)
				{
					boost::algorithm::to_lower(ocr);
					ocr[0] = toupper(ocr[0]);
					auto o = std::make_unique<rttr::type>(rttr::type::get_by_name(ocr.c_str()));
					return o->create().get_value<OcrFactory*>();
				}

				template<typename T>
				void SetFactoryProperty(std::string prop, T param)
				{
					auto p = rttr::type::get(*this).get_property(prop.c_str());
					p.set_value(this, param);
				}
				/**
				* \brief attribue une valeur à une des propriétés au moteur d'OCR créer par CreateFactory
				* \param prop nom de la propriété
				* \param param valeur a attribuer
				*/
				void SetFactoryProperty(std::string prop, std::string param)
				{
					auto p = rttr::type::get(*this).get_property(prop.c_str());
					if (p.get_type().is_enumeration())
					{
						char* p_s;
						long converted = strtol(param.c_str(), &p_s, 10);
						if (*p_s)
						{
							auto val = p.get_type().get_enumeration().name_to_value(param);

							p.set_value(this, val);
						}
						else
						{
							auto str = p.get_type().get_enumeration().get_names()[converted];
							auto val = p.get_type().get_enumeration().name_to_value(str);
							p.set_value(this, val);
						}
					}
					else
					{
						const auto type = p.get_type();
						rttr::variant var(param);
						if (var.get_type() != type)
							auto res = var.convert(type);
						p.set_value(this, var);
					}
				}
				/**
				* \brief attribue une valeur à une des propriétés au moteur d'OCR créer par CreateFactory
				* \param prop nom de la propriété
				* \param param valeur a attribuer
				*/
				void SetFactoryProperty(std::string prop, int param)
				{
					auto p = rttr::type::get(*this).get_property(prop.c_str());
					p.set_value(this, rttr::variant(param));
				}
				/**
				* \brief récupère la valeur d'une des propriétés du moteur d'OCR créer par CreateFactory
				* \param prop nom de la propriété
				* \return valeur de la propriété
				*/
				std::string GetFactoryProperty(std::string prop)
				{
					auto p = rttr::type::get(*this).get_property(prop.c_str());
					return p.get_value(this).to_string();
				}
				/**
				 * \brief Liste l'ensemble des propriété du moteur d'OCR
				 * \param ocr nom du moteur d'ocr
				 * \return liste des propriété
				 */
				static std::list<OcrProperty*> GetOcrParametersDefinition(std::string ocr)
				{
					boost::algorithm::to_lower(ocr);
					ocr[0] = toupper(ocr[0]);
					rttr::type classs_type = rttr::type::get_by_name(ocr.c_str());
					return _GetOcrParametersDefinition(classs_type);
				}

				/**
				* \brief Liste l'ensemble des propriété du moteur d'OCR actuel
				* \return liste des propriété
				*/
				std::list<OcrProperty*> GetOcrParametersDefinition()
				{
					rttr::type classs_type = rttr::type::get(*this);
					return _GetOcrParametersDefinition(classs_type);
				}

				static bool PropertyExistInOcr(std::string ocr, std::string prop)
				{
					auto o = rttr::type::get_by_name(ocr.c_str());
					if (!o)
						return false;
					auto p = o.get_property(prop.c_str());
					if (!p)
						return false;

					return true;
				}
				bool PropertyExist(std::string ocr, std::string prop) const
				{
					auto o = rttr::type::get(*this);
					if (!o)
						return false;
					auto p = o.get_property(prop.c_str());
					if (!p)
						return false;

					return true;
				}

				virtual std::string Name() = 0;
				virtual std::string Version() = 0;


				virtual ~OcrFactory() = default;
				virtual Ocr* CreateNew() = 0;
				virtual std::vector<std::string>& GetTextExtension()
				{
					return mExtension;
				}

				ImageFormatEnum ImageFormat() const { return mImageFormat; }
				void ImageFormat(const ImageFormatEnum image_format) { mImageFormat = image_format; }

				std::string GetExtension()
				{
					if (mImageFormat == ImageFormatEnum::JPG) return ".jpg";
					if (mImageFormat == ImageFormatEnum::PNG) return ".png";
					return "";
				}
				RTTR_ENABLE();
			};
		}
	}
}
