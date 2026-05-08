using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Net.Http;
using System.Security.Claims;
using System;
using System.Text;
using System.Text.Encoding;
using System.Text.Json;
using System.Text.Json.Nodes;
//using System.Web.Script.Serialization;


namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    public string name;
    public string ProductID;

    public async Task<IActionResult> OnPostAsync()
    {
        //string authentikUserID = User.FindFirst(ClaimTypes.NameIdentifier)?.Value;
        string authentikUserID = User.FindFirstValue("sub");
        /*
                var content = new JsonContent
                {
                    ["name"] = name,
                    ["ProductID"] = ProductID,
                    ["authentikUserID"] = authentikUserID
                };

                */

        using StringContent jsonContent = new(
         JsonSerializer.Serialize(new
         {
             name = name,
             ProductID = ProductID,
             authentikUserID = authentikUserID
         }),
        Encoding.UTF8,
        "application/json");

        var client = new HttpClient();
        using HttpResponseMessage response = await client.PostAsync("smakdb.head9x.dk/boards", jsonContent);

        return RedirectToPage("./Index");
    }
}
