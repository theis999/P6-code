using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Security.Claims.ClaimTypes.NameIdentifier;
using System.Text.Json;
//using System.Web.Script.Serialization;


namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    public string name;
    public string ProductID;




    public async Task<IActionResult> OnPostAsync()
    {
        string authentikUserID = User.FindFirst(ClaimTypes.NameIdentifier)?.Value;

        var content = new JsonObject
        {
            ["name"] = name,
            ["ProductID"] = ProductID,
            ["authentikUserID"] = authentikUserID
        };

        await client.PostAsync("smakdb.head9x.dk/boards", content.ToJsonString());

    }
}
